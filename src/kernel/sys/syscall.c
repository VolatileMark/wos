#include "scheduler.h"
#include "ipc/spp.h"
#include "../mem/kheap.h"
#include "../utils/macros.h"
#include "../ext/pci.h"
#include <stdint.h>
#include <stddef.h>
#include <alloc.h>
#include <mem.h>

#define POP_STACK(T, stack) *((T*) stack++)
#define INVALID_SYSCALL -128

typedef int (*syscall_handler_t)(uint64_t* stack_ptr);

extern void switch_to_kernel(cpu_state_t* state);

static void* pmalloc(uint64_t size)
{
    return (void*) allocate_heap_memory(get_current_scheduled_process()->heap, size);
}

/*
static void pfree(void* addr)
{
    free_heap_memory(get_current_scheduled_process()->heap, (uint64_t) addr);
}
*/

static int sys_exit(uint64_t* stack)
{
    UNUSED(stack);
    switch_to_kernel(&(get_current_scheduled_process()->user_mode));
    terminate_process(get_current_scheduled_process());
    run_scheduler();
    return -1;
}

static int sys_execve(uint64_t* stack)
{
    const char* path;
    process_t* current;
    process_t* new;

    path = POP_STACK(const char*, stack);
    current = get_current_scheduled_process();
    new = create_replacement_process(current, NULL);

    UNUSED(path);
    UNUSED(current);
    UNUSED(new);

    return 0;
}

static int sys_heap_expand(uint64_t* stack)
{
    uint64_t size = POP_STACK(uint64_t, stack);
    uint64_t* out_size = POP_STACK(uint64_t*, stack);
    process_t* ps = get_current_scheduled_process();
    *out_size = expand_process_heap(ps, size);
    return 0;
}

static int sys_heap_set(uint64_t* stack)
{
    heap_t* heap = POP_STACK(heap_t*, stack);
    process_t* ps = get_current_scheduled_process();
    ps->heap = heap;
    return 0;
}

static int sys_spp_expose(uint64_t* stack)
{
    uint64_t fn_addr = POP_STACK(uint64_t, stack);
    const char* fn_name = POP_STACK(const char*, stack);
    return expose_function(get_current_scheduled_process(), fn_addr, fn_name);
}

static int sys_spp_request(uint64_t* stack)
{
    const char* fn_name = POP_STACK(const char*, stack);
    uint64_t* out_addr = POP_STACK(uint64_t*, stack);
    *out_addr = request_func(get_current_scheduled_process(), fn_name);
    return -(out_addr == 0);
}

static int sys_pci_find(uint64_t* stack)
{
    pci_devices_list_entry_t* entry;
    pci_export_list_entry_t* new_entry;
    pci_header_common_t* header;
    uint64_t header_size, devices_found = 0;
    uint8_t class = POP_STACK(uint8_t, stack);
    uint8_t subclass = POP_STACK(uint8_t, stack);
    uint8_t program_interface = POP_STACK(uint8_t, stack);
    pci_export_list_t* new_list = POP_STACK(pci_export_list_t*, stack);
    pci_devices_list_t* list = find_pci_devices(class, subclass, program_interface);

    if (list == NULL)
        return 0;
    
    new_list->head = NULL;
    new_list->tail = NULL;

    entry = list->head;
    while (entry != NULL)
    {   
        /* The problem with mapping only one page is if the header overlaps two pages (very unlikely scenario but still possible) */
        header = (pci_header_common_t*) (kernel_map_temporary_page(entry->header_paddr, PAGE_ACCESS_RO, PL0) + GET_ADDR_OFFSET(entry->header_paddr));
        switch (GET_HEADER_TYPE(header->header_type))
        {
        case PCI_HEADER_0x0:
            header_size = sizeof(pci_header_0x0_t);
            break;
        case PCI_HEADER_0x1:
            header_size = sizeof(pci_header_0x1_t);
            break;
        case PCI_HEADER_0x2:
            header_size = sizeof(pci_header_0x2_t);
            break;
        }
        
        new_entry = pmalloc(header_size + sizeof(pci_devices_list_entry_t*));
        if (new_entry == NULL)
        {
            kernel_unmap_temporary_page((uint64_t) header);
            delete_devices_list(list);
            free(list);
            return 0;
        }
        new_entry->next = NULL;
        memcpy(&new_entry->header, header, header_size);
        if (new_list->tail == NULL)
            new_list->head = new_entry;
        else
            new_list->tail->next = new_entry;
        new_list->tail = new_entry;

        kernel_unmap_temporary_page((uint64_t) header);
        entry = entry->next;
        ++devices_found;
    }

    delete_devices_list(list);
    return devices_found;
}

static syscall_handler_t handlers[] = {
    sys_exit,
    sys_execve,
    sys_heap_expand,
    sys_heap_set,
    sys_spp_expose,
    sys_spp_request,
    sys_pci_find
};

#define NUM_SYSCALLS (sizeof(handlers) / sizeof(uint64_t))

int syscall_handler(uint64_t num, uint64_t* stack)
{
    if (num < NUM_SYSCALLS)
        return handlers[num](stack);
    return INVALID_SYSCALL;
}