#include "kheap.h"
#include "pfa.h"
#include "paging.h"
#include "../utils/constants.h"
#include <stddef.h>
#include <math.h>
#include <mem.h>
#include <alloc.h>

extern int init_alloc
(
    expand_heap_func_t expand_heap_func, 
    uint64_t start_vaddr, 
    uint64_t ceil_vaddr, 
    uint64_t initial_size, 
    PRIVILEGE_LEVEL privilege_level
);

static void record_segment(segment_list_t* sl, uint64_t paddr, uint64_t pages)
{
    segment_list_entry_t* entry = malloc(sizeof(segment_list_entry_t));
    
    entry->paddr = paddr;
    entry->pages = pages;
    entry->next = NULL;

    if (sl->head == NULL)
        sl->head = entry;
    else
        sl->tail->next = entry;
    sl->tail = entry;
}

static uint64_t expand_heap(heap_t* heap, page_table_t pml4, uint64_t* in_out_size)
{
    uint64_t pages_paddr, pages_count, mapped_size, new_size;
    heap_segment_header_t* new = (heap_segment_header_t*) heap->end_vaddr;
    
    new_size = alignu(*in_out_size + sizeof(heap_segment_header_t), SIZE_4KB);
    if (heap->end_vaddr + new_size > heap->ceil_vaddr)
        return 0;
    
    pages_count = ceil((double) new_size / SIZE_4KB);
    pages_paddr = request_pages(pages_count);
    if (pages_paddr == 0)
    {
        *in_out_size = 0;
        return 0;
    }
    
    mapped_size = pml4_map_memory(pml4, pages_paddr, heap->end_vaddr, new_size, PAGE_ACCESS_RW, heap->privilege_level);
    if (mapped_size < new_size)
    {
        free_pages(pages_paddr, pages_count);
        *in_out_size = 0;
        return 0;
    }

    heap->end_vaddr += mapped_size;

    new->free = 1;
    new->size = new_size;
    new->data = (uint64_t)(new + 1);
    new->prev = heap->tail;
    if (heap->tail != NULL)
    {
        new->next = heap->tail->next;
        heap->tail->next = new;
    }
    heap->tail = new;
    
    *in_out_size = mapped_size;
    return pages_paddr;
}

static uint64_t expand_kernel_heap(heap_t* heap, uint64_t size)
{
    expand_heap(heap, get_kernel_pml4(), &size);
    return size;
}

int init_kernel_heap(uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t inital_size)
{
     return init_alloc(expand_kernel_heap, start_vaddr, ceil_vaddr, inital_size, PL0);
}

uint64_t expand_process_heap(process_t* ps, heap_t* heap, uint64_t size)
{
    uint64_t pages_num = ceil((double) size / SIZE_4KB);
    uint64_t pages_paddr = expand_heap(heap, ps->pml4, &size);
    record_segment(&ps->heap_segments, pages_paddr, pages_num);
    return size;
}