#include "heap.h"
#include "pfa.h"
#include "paging.h"
#include "../utils/constants.h"
#include <mem.h>
#include <math.h>
#include <stddef.h>

#define MIN_SIZE 16

static uint64_t heap_start, heap_end, max_size;
static heap_segment_header_t* head;
static heap_segment_header_t* tail;

static uint64_t expand_heap(uint64_t size)
{
    uint64_t pages_paddr;
    heap_segment_header_t* new = (heap_segment_header_t*) heap_end;
    
    size = alignu(size + sizeof(heap_segment_header_t*), SIZE_4KB);
    pages_paddr = request_pages(size / SIZE_4KB);
    if (pages_paddr == 0)
        return 0;
    
    size = paging_map_memory(pages_paddr, heap_end, size, PAGE_ACCESS_RW, PL0);
    heap_end += size;

    new->free = 1;
    new->size = size - sizeof(heap_segment_header_t*);
    new->data = (uint64_t)(new + 1);
    new->prev = tail;
    new->next = tail->next;
    tail->next = new;
    tail = new;

    return size;
}

static heap_segment_header_t* next_free_segment(uint64_t size)
{
    heap_segment_header_t* current = head;
    while (current != NULL)
    {
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    expand_heap(size);
    return next_free_segment(size);
}

static void heap_combine_forward(heap_segment_header_t* seg)
{
    if (seg->next == NULL || !seg->next->free)
        return;
    if (seg->next == tail)
        tail = seg;
    seg->size += seg->next->size + sizeof(heap_segment_header_t);
    seg->next = seg->next->next;
}

static void heap_combine_backward(heap_segment_header_t* seg)
{
    if (seg->prev != NULL)
        heap_combine_forward(seg->prev);
}

void free_heap_memory(uint64_t addr)
{
    heap_segment_header_t* seg = (heap_segment_header_t*) (addr - sizeof(heap_segment_header_t));
    seg->free = 1;
    heap_combine_forward(seg);
    heap_combine_backward(seg);
}

uint64_t allocate_heap_memory(uint64_t size)
{
    heap_segment_header_t* seg;
    heap_segment_header_t* new;

    size = alignu(size, MIN_SIZE) + sizeof(heap_segment_header_t);

    seg = next_free_segment(size);
    if (seg == NULL)
        return 0;
    
    new = (heap_segment_header_t*)(seg->data + size);
    new->free = 1;
    new->size = seg->size - size - sizeof(heap_segment_header_t);
    new->prev = seg;
    new->next = seg->next;
    new->next->prev = new;
    new->data = (uint64_t)(new + 1);
    if (seg == tail)
        tail = new;
    
    seg->free = 0;
    seg->size = size;
    seg->next = new;

    return seg->data;
}

int init_heap(uint64_t start_addr, uint64_t end_addr, uint64_t initial_pages)
{
    uint64_t size, pages_paddr;
    
    max_size = end_addr - start_addr;
    size = initial_pages * SIZE_4KB;
    if (size > max_size)
    {
        size = max_size;
        initial_pages = ceil((double) size / SIZE_4KB);
    }
    pages_paddr = request_pages(initial_pages);
    
    if 
    (
        pages_paddr == 0 || 
        paging_map_memory(pages_paddr, start_addr, size, PAGE_ACCESS_RW, PL0) < size
    )
        return -1;
    
    heap_start = start_addr;
    heap_end = start_addr + size;
    
    head = (heap_segment_header_t*) heap_start;
    head->next = NULL;
    head->prev = NULL;
    head->data = (uint64_t)(head + 1);
    head->size = size - sizeof(heap_segment_header_t);
    head->free = 1;

    tail = head;

    return 0;
}