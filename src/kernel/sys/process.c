#include "process.h"
#include "scheduler.h"
#include "../cpu/gdt.h"
#include "../utils/alloc.h"
#include "../utils/constants.h"
#include "../mem/pfa.h"
#include "../mem/paging.h"
#include "../utils/elf.h"
#include "../utils/macros.h"
#include <stddef.h>
#include <mem.h>
#include <math.h>

#define PROC_INITIAL_STACK_PAGES 1
#define PROC_INITIAL_HEAP_PAGES 10
#define PROC_INITIAL_RSP (PML4_VADDR - sizeof(uint64_t))
#define PROC_DEFAULT_STACK_VADDR (PML4_VADDR - SIZE_4KB)
#define PROC_DEFAULT_HEAP_VADDR VADDR_GET(256, 0, 0, 0)
#define PROC_DEFAULT_RFLAGS 0x202 /* Only interrupts enabled */

static void init_process(process_t* ps, uint64_t pid)
{
    ps->pid = pid;
    ps->parent_pid = 0;
    ps->pml4 = NULL;
    ps->pml4_paddr = 0;
    ps->kernel_stack_start_vaddr = 0;
    ps->code_start_vaddr = 0;
    ps->stack_start_vaddr = PROC_DEFAULT_STACK_VADDR;
    ps->kernel_stack_segments.head = NULL;
    ps->kernel_stack_segments.tail = NULL;
    ps->code_segments.head = NULL;
    ps->code_segments.tail = NULL;
    ps->stack_segments.head = NULL;
    ps->stack_segments.tail = NULL;

    memset(&ps->heap, 0, sizeof(process_heap_t));
    memset(ps->fds, 0, PROCESS_MAX_FDS * sizeof(file_descriptor_t));
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
        paging_get_next_vaddr(SIZE_4KB, &vaddr) < SIZE_4KB ||
        paging_map_memory(paddr, vaddr, SIZE_4KB, PAGE_ACCESS_RW, PL0) < SIZE_4KB
    )
        return -1;

    ps->pml4 = (page_table_t) vaddr;
    ps->pml4_paddr = paddr;
    memset(ps->pml4, 0, SIZE_4KB);

    return 0;
}

static int process_load_binary(process_t* ps, const process_file_t* file, uint64_t vaddr)
{
    uint64_t size;
    segment_list_entry_t* code_segments;

    size = pml4_map_memory(ps->pml4, file->paddr, vaddr, file->size, PAGE_ACCESS_RW, PL3);
    if (size < file->size)
        return -1;
    
    code_segments = kmalloc(sizeof(segment_list_entry_t));
    if (code_segments == NULL)
        return -1;
    
    code_segments->paddr = file->paddr;
    code_segments->pages = ceil((double) file->size / SIZE_4KB);
    code_segments->next = NULL;

    ps->code_segments.head = code_segments;
    ps->code_segments.tail = code_segments;
    ps->user_mode.stack.rip = vaddr;
    ps->code_start_vaddr = vaddr;
    
    return 0;
}

static int process_load_elf(process_t* ps, const process_file_t* file)
{
    UNUSED(ps);
    UNUSED(file);
    return 0;
}

static int load_process_code(process_t* ps, const process_file_t* file)
{
    switch (file->type)
    {
    case PROC_EXEC_BIN:
        return process_load_binary(ps, file, 0);
    case PROC_EXEC_ELF:
        return process_load_elf(ps, file);
    }
    return -1;
}

static int init_process_stack(process_t* ps)
{
    uint64_t paddr, bytes, size;
    segment_list_entry_t* stack_segments;

    paddr = request_pages(PROC_INITIAL_STACK_PAGES);
    if (paddr == 0)
        return -1;
    
    bytes = PROC_INITIAL_STACK_PAGES * SIZE_4KB;
    size = pml4_map_memory(ps->pml4, paddr, PROC_DEFAULT_STACK_VADDR, bytes, PAGE_ACCESS_RW, PL3);
    if (size < bytes)
        return -1;

    stack_segments = kmalloc(sizeof(segment_list_entry_t));
    if (stack_segments == NULL)
        return -1;
    
    stack_segments->paddr = paddr;
    stack_segments->pages = bytes;
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

    pages = ceil((double) KERNEL_STACK_SIZE / SIZE_4KB);
    paddr = request_pages(pages);
    if (paddr == 0)
        return -1;
    
    bytes = pages * SIZE_4KB;
    size = paging_get_next_vaddr(bytes, &vaddr);
    if (size < bytes)
        return -1;
    
    size = paging_map_memory(paddr, vaddr, bytes, PAGE_ACCESS_RW, PL0);
    if (size < bytes)
        return -1;
    
    kernel_stack_segments = kmalloc(sizeof(segment_list_entry_t));
    if (kernel_stack_segments == NULL)
        return -1;
    
    kernel_stack_segments->paddr = paddr;
    kernel_stack_segments->pages = pages;
    kernel_stack_segments->next = NULL;

    ps->kernel_stack_segments.head = kernel_stack_segments;
    ps->kernel_stack_segments.tail = kernel_stack_segments;
    ps->kernel_stack_start_vaddr = vaddr + bytes - sizeof(uint64_t);

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
        kfree(current);
        current = tmp;
    }

    return pages * SIZE_4KB;
}

void delete_process_resources(process_t* ps)
{
    uint64_t size, i;

    if (ps->pml4 != 0)
    {
        if (ps->pml4 == get_current_scheduled_process()->pml4)
            load_kernel_pml4();
        delete_pml4(ps->pml4);
    }
    
    if (ps->kernel_stack_start_vaddr != 0)
    {
        size = delete_segments_list(&ps->kernel_stack_segments);
        paging_unmap_memory(ps->kernel_stack_start_vaddr, size);
    }

    delete_segments_list(&ps->code_segments);
    delete_segments_list(&ps->stack_segments);
    delete_segments_list(&ps->heap.segments);

    for (i = 0; i < PROCESS_MAX_FDS; i++)
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
    kfree(ps);
}

static int setup_process_mailbox(process_t* ps)
{
    return 0;
}

process_t* create_process(const process_file_t* file, uint64_t pid)
{
    process_t* ps = kmalloc(sizeof(process_t));
    if (ps == NULL)
        return NULL;

    init_process(ps, pid);

    if 
    (
        init_process_pml4(ps) ||
        load_process_code(ps, file) || 
        init_process_stack(ps) || 
        init_process_kernel_stack(ps) || 
        init_process_heap(PROC_DEFAULT_HEAP_VADDR, KERNEL_HEAP_START_ADDR, PROC_INITIAL_HEAP_PAGES, ps) < (PROC_INITIAL_HEAP_PAGES * SIZE_4KB) ||
        setup_process_mailbox(ps)
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
    for (i = 0; i < PROCESS_MAX_FDS; i++)
    {
        if (from[i] != 0)
        {
            /* Implement file descriptor copy */
            to[i] = from[i];
        }
    }
    return 0;
}

process_t* create_replacement_process(process_t* parent, const process_file_t* file)
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
        
        size = paging_get_next_vaddr(bytes, &kernel_vaddr);
        if (size < bytes)
        {
            free_pages(paddr, ptr->pages);
            return -1;
        }
        
        size = paging_map_memory(paddr, kernel_vaddr, bytes, PAGE_ACCESS_RW, PL0);
        if (size < bytes)
        {
            paging_unmap_memory(kernel_vaddr, bytes);
            free_pages(paddr, ptr->pages);
            return -1;
        }
        
        memcpy((void*) kernel_vaddr, (void*) vaddr, bytes);
        paging_unmap_memory(kernel_vaddr, bytes);
        
        segment = kmalloc(sizeof(segment_list_entry_t));
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
            kfree(segment);
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
    process_t* child = kmalloc(sizeof(process_t));
    if (child == NULL)
        return NULL;
    
    init_process(child, id);

    child->parent_pid = parent->pid;
    child->user_mode = parent->user_mode;
    child->code_start_vaddr = parent->code_start_vaddr;
    child->stack_start_vaddr = parent->stack_start_vaddr;

    child->heap.start_vaddr = parent->heap.start_vaddr;
    child->heap.end_vaddr = parent->heap.end_vaddr;
    child->heap.max_size = parent->heap.max_size;
    child->heap.head = parent->heap.head;
    child->heap.tail = parent->heap.tail;

    if 
    (
        init_process_pml4(child) ||
        copy_segments_list(child->pml4, parent->code_start_vaddr, &parent->code_segments, &child->code_segments) ||
        copy_segments_list(child->pml4, parent->stack_start_vaddr, &parent->stack_segments, &child->stack_segments) ||
        copy_segments_list(child->pml4, parent->heap.start_vaddr, &parent->heap.segments, &child->heap.segments) ||
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
    paging_inject_pml4(ps->pml4);
    load_pml4(ps->pml4_paddr);
}