#include "spp.h"
#include "../../utils/constants.h"
#include "../../utils/helpers/alloc.h"
#include "../../mem/paging.h"
#include <mem.h>
#include <string.h>
#include <stddef.h>

#define NOP 0x90
#define LEAVEQ 0xC9
#define RETQ 0xC3
#define MAX_EXPOSED_FUNCS 10
#define MAX_FUNC_SIZE SIZE_4KB

/*
#define SPPEND() __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop"); \
                 __asm__ __volatile__ ("nop");

#define SPPFUNC __attribute__((section(".exposed")))
*/

#define SPPFUNC __attribute__((section(".exposed"))) void

typedef struct
{
    const char* name;
    uint64_t paddr;
    uint64_t size;
} exposed_function_properties_t;

typedef struct processes_list_entry
{
    struct processes_list_entry* next;
    process_t* ps;
    uint64_t num_exposed_funcs;
    exposed_function_properties_t exposed_funcs[MAX_EXPOSED_FUNCS];
} processes_list_entry_t;

typedef struct
{
    processes_list_entry_t* head;
    processes_list_entry_t* tail;
} processes_list_t;

static processes_list_t proc_list;

void init_spp(void)
{
    memset(&proc_list, 0, sizeof(processes_list_t));
}

static processes_list_entry_t* register_process(process_t* ps)
{
    processes_list_entry_t* new = malloc(sizeof(processes_list_entry_t));
    new->next = NULL;
    new->ps = ps;
    new->num_exposed_funcs = 0;
    memset(new->exposed_funcs, 0, MAX_EXPOSED_FUNCS * sizeof(exposed_function_properties_t));
    if (proc_list.tail == NULL)
        proc_list.head = new;
    else
        proc_list.tail->next = new;
    proc_list.tail = new;
    return new;
}

static processes_list_entry_t* get_process_list_entry(process_t* ps)
{
    processes_list_entry_t* entry = proc_list.head;
    while (entry != NULL)
    {
        if (entry->ps == ps)
            return entry;
        entry = entry->next;
    }
    return register_process(ps);
}

static uint64_t get_function_size(uint64_t func_addr)
{
    uint8_t* opcode = (uint8_t*) func_addr;
    for (; (((uint64_t) opcode) - func_addr) < MAX_FUNC_SIZE; opcode++)
    {
        if 
        (
            opcode[0] == NOP &&
            opcode[1] == LEAVEQ &&
            opcode[2] == RETQ
        )
            return (((uint64_t) &opcode[2]) - func_addr);
    }
    return 0;
}

int expose_function(process_t* ps, uint64_t func_vaddr, const char* func_name)
{
    exposed_function_properties_t* fn;
    processes_list_entry_t* entry = get_process_list_entry(ps);
    uint64_t name_size = sizeof(uint8_t) * (strlen(func_name) + 1);
    
    if (entry->num_exposed_funcs >= MAX_EXPOSED_FUNCS)
        return -1;
    
    fn = &entry->exposed_funcs[entry->num_exposed_funcs];
    
    fn->size = get_function_size(func_vaddr);
    if (fn->size == 0)
        return -1;

    fn->paddr = pml4_get_paddr(ps->pml4, func_vaddr);
    if (fn->paddr == 0)
        return -1;

    fn->name = malloc(name_size);
    if (fn->name == NULL)
        return -1;
    memcpy((void*) fn->name, func_name, name_size);
        
    ++entry->num_exposed_funcs;

    return 0;
}

void delete_exposed_functions(process_t* ps)
{
    uint64_t i;
    processes_list_entry_t* prev = NULL;
    processes_list_entry_t* entry = proc_list.head;
    while (entry != NULL)
    {
        if (entry->ps == ps)
        {
            if (prev == NULL)
            {
                if (proc_list.head == proc_list.tail)
                    proc_list.tail = NULL;
                proc_list.head = entry->next;
            }
            else
                prev->next = entry->next;
            for (i = 0; i < entry->num_exposed_funcs; i++)
                free((void*) entry->exposed_funcs[i].name);
            free(entry);
            return;
        }

        prev = entry;
        entry = prev->next;
    }
}

uint64_t request_func(process_t* ps, const char* name)
{
    uint64_t i, fn_vaddr;
    exposed_function_properties_t* fn;
    processes_list_entry_t* entry = proc_list.head;
    while (entry != NULL)
    {
        for (i = 0; i < entry->num_exposed_funcs; i++)
        {
            fn = &entry->exposed_funcs[i];
            if (strcmp(fn->name, name) == 0)
            {
                if 
                (
                    pml4_get_next_vaddr(ps->pml4, ps->code_start_vaddr, fn->size, &fn_vaddr) < fn->size ||
                    pml4_map_memory(ps->pml4, fn->paddr, fn_vaddr, fn->size, PAGE_ACCESS_RO, PL3) < fn->size
                )
                    return 0;
                return (fn_vaddr + GET_ADDR_OFFSET(fn->paddr));
            }
        }
        entry = entry->next;
    }
    return 0;
}