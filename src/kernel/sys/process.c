#include "process.h"
#include "scheduler.h"
#include "../cpu/gdt.h"
#include "../utils/constants.h"
#include "../mem/pfa.h"
#include "../mem/paging.h"
#include "../utils/elf.h"
#include "../utils/macros.h"
#include <stddef.h>
#include <mem.h>
#include <math.h>
#include <alloc.h>
#include <string.h>

static void init_process(process_t* ps, uint64_t pid)
{
    ps->pid = pid;
    ps->parent_pid = 0;
    
    ps->pml4 = NULL;
    ps->pml4_paddr = 0;
    
    ps->kernel_stack_start_vaddr = 0;
    ps->code_start_vaddr = 0;
    ps->args_start_vaddr = 0;
    ps->stack_start_vaddr = PROC_DEFAULT_STACK_VADDR;
    ps->heap_start_vaddr = PROC_DEFAULT_HEAP_VADDR;
    
    ps->kernel_stack_segments.head = NULL;
    ps->kernel_stack_segments.tail = NULL;
    
    ps->code_segments.head = NULL;
    ps->code_segments.tail = NULL;
    
    ps->stack_segments.head = NULL;
    ps->stack_segments.tail = NULL;
    
    ps->heap_segments.head = NULL;
    ps->heap_segments.tail = NULL;
    
    ps->args_segments.head = NULL;
    ps->args_segments.tail = NULL;

    memset(&ps->flags, 0, sizeof(process_flags_t));
    memset(ps->fds, 0, PROC_MAX_FDS * sizeof(file_descriptor_t));
    memset(&ps->user_mode, 0, sizeof(cpu_state_t));
    memset(&ps->current, 0, sizeof(cpu_state_t));

    ps->user_mode.stack.rflags = PROC_DEFAULT_RFLAGS;
    ps->user_mode.stack.cs = get_user_cs() | PL3;
    ps->user_mode.stack.ss = get_user_ds() | PL3;
}

static int init_process_pml4(process_t* ps)
{
    uint64_t paddr, vaddr;

    paddr = request_page();
    if 
    (
        paddr == 0 || 
        kernel_get_next_vaddr(SIZE_4KB, &vaddr) < SIZE_4KB ||
        kernel_map_memory(paddr, vaddr, SIZE_4KB, PAGE_ACCESS_RW, PL0) < SIZE_4KB
    )
        return -1;

    ps->pml4 = (page_table_t) vaddr;
    ps->pml4_paddr = paddr;
    memset(ps->pml4, 0, SIZE_4KB);

    return 0;
}

static int process_load_binary(process_t* ps, const process_descriptor_t* desc, uint64_t vaddr)
{
    uint64_t size;
    segment_list_entry_t* code_segments;

    size = pml4_map_memory(ps->pml4, desc->exec_paddr, vaddr, desc->exec_size, PAGE_ACCESS_RW, PL3);
    if (size < desc->exec_size)
        return -1;
    
    code_segments = malloc(sizeof(segment_list_entry_t));
    if (code_segments == NULL)
        return -1;
    
    code_segments->paddr = desc->exec_paddr;
    code_segments->pages = ceil((double) desc->exec_size / SIZE_4KB);
    code_segments->next = NULL;

    ps->code_segments.head = code_segments;
    ps->code_segments.tail = code_segments;
    ps->user_mode.stack.rip = vaddr;
    ps->code_start_vaddr = vaddr;
    
    return 0;
}

static int process_load_elf(process_t* ps, const process_descriptor_t* desc)
{
    UNUSED(ps);
    UNUSED(desc);
    return 0;
}

static int load_process_code(process_t* ps, const process_descriptor_t* desc)
{
    switch (desc->exec_type)
    {
    case PROC_EXEC_BIN:
        return process_load_binary(ps, desc, 0);
    case PROC_EXEC_ELF:
        return process_load_elf(ps, desc);
    }
    return -1;
}

static int init_process_stack(process_t* ps)
{
    uint64_t paddr, bytes, pages, size;
    segment_list_entry_t* stack_segments;

    pages = ceil((double) PROC_INITIAL_STACK_SIZE / SIZE_4KB);
    paddr = request_pages(PROC_INITIAL_STACK_SIZE);
    if (paddr == 0)
        return -1;
    
    bytes = pages * SIZE_4KB;
    size = pml4_map_memory(ps->pml4, paddr, PROC_DEFAULT_STACK_VADDR, bytes, PAGE_ACCESS_RW, PL3);
    if (size < bytes)
    {
        free_pages(paddr, pages);
        return -1;
    }

    stack_segments = malloc(sizeof(segment_list_entry_t));
    if (stack_segments == NULL)
    {
        free_pages(paddr, pages);
        return -1;
    }
    
    stack_segments->paddr = paddr;
    stack_segments->pages = pages;
    stack_segments->next = NULL;

    ps->stack_segments.head = stack_segments;
    ps->stack_segments.tail = stack_segments;
    ps->stack_start_vaddr = PROC_DEFAULT_STACK_VADDR;
    ps->user_mode.stack.rsp = PROC_INITIAL_RSP;

    return 0;
}

static int init_process_kernel_stack(process_t* ps)
{
    uint64_t pages, bytes, vaddr, paddr, size;
    segment_list_entry_t* kernel_stack_segments;

    bytes = PROC_INITIAL_STACK_SIZE;
    size = kernel_get_next_vaddr(bytes, &vaddr);
    if (size < bytes)
        return -1;

    pages = ceil((double) PROC_INITIAL_STACK_SIZE / SIZE_4KB);
    paddr = request_pages(pages);
    if (paddr == 0)
        return -1;
    
    size = pml4_map_memory(ps->pml4, paddr, vaddr, bytes, PAGE_ACCESS_RW, PL0);
    if (size < bytes)
    {
        free_pages(paddr, pages);
        return -1;
    }
    
    kernel_stack_segments = malloc(sizeof(segment_list_entry_t));
    if (kernel_stack_segments == NULL)
    {
        free_pages(paddr, pages);
        return -1;
    }
    
    kernel_stack_segments->paddr = paddr;
    kernel_stack_segments->pages = pages;
    kernel_stack_segments->next = NULL;

    ps->kernel_stack_segments.head = kernel_stack_segments;
    ps->kernel_stack_segments.tail = kernel_stack_segments;
    ps->kernel_stack_start_vaddr = vaddr;

    return 0;
}

static char** parse_args(char* c, uint64_t* argc, uint64_t* bytes)
{
    char** argv;
    uint64_t i;

    i = 0;
    *argc = 1;
    *bytes = strlen(c);

#define SINGLE_QUOTE_OPEN_BIT (1 << 0)
#define DOUBLE_QUOTE_OPEN_BIT (1 << 1)

    while (*c != '\0')
    {
        switch (*c)
        {
        case '"':
            i ^= DOUBLE_QUOTE_OPEN_BIT;
            break;
        case '\'':
            i ^= SINGLE_QUOTE_OPEN_BIT;
            break;
        case ' ':
            if ((i & (DOUBLE_QUOTE_OPEN_BIT | SINGLE_QUOTE_OPEN_BIT)))
                break;
            *c = '\0';
            *argc += 1;
            break;
        }
        ++c;
    }

    argv = calloc(1, *argc * sizeof(char*));
    
    for 
    (
        c = c - *bytes,
        i = 0;
        i < *argc;
        c++
    )
    {
        if (argv[i] == NULL)
            argv[i] = c;
        if (*c == '\0')
        {
            if (*(c - 1) == '\'' || *(c - 1) == '"')
            {
                ++argv[i];
                *(c - 1) = '\0';
            }
            ++i;
        }
    }

    return argv;
}

static int load_process_args(process_t* ps, const process_descriptor_t* desc)
{
    segment_list_entry_t* args_segments;
    uint64_t argc;
    uint64_t cmdline_bytes, argv_bytes, total_bytes, pages, size;
    uint64_t paddr, vaddr, tmp_vaddr;
    char** argv;

    if (desc->cmdline == NULL)
        return 0;
    
    argv = parse_args(desc->cmdline, &argc, &cmdline_bytes);
    argv_bytes = argc * sizeof(char*);

    if (cmdline_bytes > SIZE_4KB)
    {
        free(argv);
        return -1;
    }

    total_bytes = cmdline_bytes + argv_bytes;
    size = pml4_get_next_vaddr(ps->pml4, ps->code_start_vaddr, total_bytes, &vaddr);
    if (size < total_bytes)
    {
        free(argv);
        return -1;
    }
    
    pages = ceil((double) total_bytes / SIZE_4KB);
    paddr = request_pages(pages);
    if (paddr == 0)
    {
        free(argv);
        return -1;
    }

    tmp_vaddr = kernel_map_temporary_page(paddr, PAGE_ACCESS_RW, PL0);
    memcpy((void*) tmp_vaddr, desc->cmdline, cmdline_bytes);
    kernel_unmap_temporary_page(tmp_vaddr);

    tmp_vaddr = kernel_map_temporary_page(paddr + cmdline_bytes, PAGE_ACCESS_RW, PL0);
    memcpy((void*) tmp_vaddr, argv, argv_bytes);
    kernel_unmap_temporary_page(tmp_vaddr);

    free(argv);

    size = pml4_map_memory(ps->pml4, paddr, vaddr, total_bytes, PAGE_ACCESS_RW, PL3);
    if (size < total_bytes)
    {
        free_pages(paddr, pages);
        return -1;
    }

    args_segments = malloc(sizeof(segment_list_entry_t));
    if (args_segments == NULL)
    {
        free_pages(paddr, pages);
        return -1;
    }

    args_segments->paddr = paddr;
    args_segments->pages = pages;
    args_segments->next = NULL;

    ps->args_segments.head = args_segments;
    ps->args_segments.tail = args_segments;
    ps->args_start_vaddr = vaddr;

    ps->user_mode.regs.rdi = argc;
    ps->user_mode.regs.rsi = ps->args_start_vaddr;

    return 0;
}

static uint64_t delete_segments_list(segment_list_t* list)
{
    segment_list_entry_t* current;
    segment_list_entry_t* tmp;
    uint64_t pages = 0;

    current = list->head;
    while (current != NULL)
    {
        pages += current->pages;
        tmp = current->next;
        free_pages(current->paddr, current->pages);
        free(current);
        current = tmp;
    }

    return pages * SIZE_4KB;
}

void delete_process_resources(process_t* ps)
{
    uint64_t i;

    if (ps->pml4 != 0)
    {
        if (delete_pml4(ps->pml4, ps->pml4_paddr) > KERNEL_HEAP_START_ADDR)
            return;
        kernel_unmap_memory((uint64_t) ps->pml4, SIZE_4KB);
    }
    
    if (ps->kernel_stack_start_vaddr != 0)
        delete_segments_list(&ps->kernel_stack_segments);

    delete_segments_list(&ps->code_segments);
    delete_segments_list(&ps->stack_segments);
    delete_segments_list(&ps->heap_segments);
    
    if (ps->args_start_vaddr != 0)
        delete_segments_list(&ps->args_segments);

    for (i = 0; i < PROC_MAX_FDS; i++)
    {
        if (ps->fds[i] != 0)
        {
            /* Free file descriptors */
        }
    }
}

void delete_and_free_process(process_t* ps)
{
    delete_process_resources(ps);
    free(ps);
}

process_t* create_process(const process_descriptor_t* desc, uint64_t pid)
{
    process_t* ps = malloc(sizeof(process_t));
    if (ps == NULL)
        return NULL;

    init_process(ps, pid);

    if 
    (
        init_process_pml4(ps) ||
        load_process_code(ps, desc) || 
        init_process_stack(ps) || 
        init_process_kernel_stack(ps) ||
        load_process_args(ps, desc)
    )
    {
        delete_and_free_process(ps);
        return NULL;
    }

    ps->current = ps->user_mode;
    return ps;
}

static int copy_file_descriptors(file_descriptor_t* from, file_descriptor_t* to)
{
    uint64_t i;
    for (i = 0; i < PROC_MAX_FDS; i++)
    {
        if (from[i] != 0)
        {
            /* Implement file descriptor copy */
            to[i] = from[i];
        }
    }
    return 0;
}

process_t* create_replacement_process(process_t* parent, const process_descriptor_t* file)
{
    process_t* child = create_process(file, parent->pid);
    if (child == NULL)
        return NULL;
    child->parent_pid = parent->parent_pid;
    if (copy_file_descriptors(parent->fds, child->fds))
    {
        delete_and_free_process(child);
        return NULL;
    }
    return child;
}

static int copy_segments_list(page_table_t pml4, uint64_t vaddr, segment_list_t* from, segment_list_t* to)
{
    uint64_t bytes, size, paddr, kernel_vaddr;
    segment_list_entry_t* segment;
    segment_list_entry_t* ptr = from->head;

    while (ptr != NULL)
    {
        paddr = request_pages(ptr->pages);
        if (paddr == 0)
            return -1;
        
        bytes = ptr->pages * SIZE_4KB;
        
        size = kernel_get_next_vaddr(bytes, &kernel_vaddr);
        if (size < bytes)
        {
            free_pages(paddr, ptr->pages);
            return -1;
        }
        
        size = kernel_map_memory(paddr, kernel_vaddr, bytes, PAGE_ACCESS_RW, PL0);
        if (size < bytes)
        {
            kernel_unmap_memory(kernel_vaddr, bytes);
            free_pages(paddr, ptr->pages);
            return -1;
        }
        
        memcpy((void*) kernel_vaddr, (void*) vaddr, bytes);
        kernel_unmap_memory(kernel_vaddr, bytes);
        
        segment = malloc(sizeof(segment_list_entry_t));
        if (segment == NULL)
        {
            free_pages(paddr, ptr->pages);
            return -1;
        }

        segment->paddr = paddr;
        segment->pages = ptr->pages;
        segment->next = NULL;

        if (to->head == NULL)
            to->head = segment;
        else
            to->tail->next = segment;
        to->tail = segment;

        size = pml4_map_memory(pml4, paddr, vaddr, bytes, PAGE_ACCESS_RW, PL3);
        if (size < bytes)
        {
            free(segment);
            free_pages(paddr, ptr->pages);
            return -1;
        }

        vaddr += bytes;
        ptr = ptr->next;
    }

    return 0;
}

process_t* clone_process(process_t* parent, uint64_t id)
{
    process_t* child = malloc(sizeof(process_t));
    if (child == NULL)
        return NULL;
    
    init_process(child, id);

    child->parent_pid = parent->pid;
    child->user_mode = parent->user_mode;

    child->code_start_vaddr = parent->code_start_vaddr;
    child->stack_start_vaddr = parent->stack_start_vaddr;
    child->heap_start_vaddr = parent->heap_start_vaddr;
    child->args_start_vaddr = parent->args_start_vaddr;

    if 
    (
        init_process_pml4(child) ||
        copy_segments_list(child->pml4, parent->code_start_vaddr, &parent->code_segments, &child->code_segments) ||
        copy_segments_list(child->pml4, parent->stack_start_vaddr, &parent->stack_segments, &child->stack_segments) ||
        copy_segments_list(child->pml4, parent->heap_start_vaddr, &parent->heap_segments, &child->heap_segments) ||
        copy_segments_list(child->pml4, parent->args_start_vaddr, &parent->args_segments, &child->args_segments) ||
        init_process_kernel_stack(child) || 
        copy_file_descriptors(parent->fds, child->fds)
    )
    {
        delete_and_free_process(child);
        return NULL;
    }

    return child;
}

void load_process_pml4(process_t* ps)
{
    kernel_inject_pml4(ps->pml4);
    load_pml4(ps->pml4_paddr);
}