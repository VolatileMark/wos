#include "heap.h"
#include "pfa.h"
#include "paging.h"
#include "../utils/constants.h"
#include "../utils/helpers/alloc.h"
#include <stddef.h>
#include <math.h>
#include <mem.h>

typedef struct heap
{
    uint64_t start_vaddr;
    uint64_t end_vaddr;
    uint64_t ceil_vaddr;
    heap_segment_header_t* head;
    heap_segment_header_t* tail;
} heap_t;

#define MIN_ALLOC_SIZE sizeof(uint64_t)

static heap_t kernel_heap;

static uint64_t expand_kernel_heap(uint64_t size)
{
    uint64_t pages_paddr, pages_count, mapped_size, new_size;
    heap_segment_header_t* new;
    
    new = (heap_segment_header_t*) kernel_heap.end_vaddr;
    
    new_size = alignu(size + sizeof(heap_segment_header_t), SIZE_4KB);
    if (kernel_heap.end_vaddr + new_size > kernel_heap.ceil_vaddr)
        return 0;
    
    pages_count = ceil((double) new_size / SIZE_4KB);
    pages_paddr = request_pages(pages_count);
    if (pages_paddr == 0)
        return 0;
    
    mapped_size = kernel_map_memory(pages_paddr, kernel_heap.end_vaddr, new_size, PAGE_ACCESS_WX, PL0);
    if (mapped_size < new_size)
    {
        free_pages(pages_paddr, pages_count);
        return 0;
    }

    kernel_heap.end_vaddr += mapped_size;

    new->free = 1;
    new->size = new_size;
    new->data = (uint64_t)(new + 1);
    new->prev = kernel_heap.tail;
    if (kernel_heap.tail != NULL)
    {
        new->next = kernel_heap.tail->next;
        kernel_heap.tail->next = new;
    }
    kernel_heap.tail = new;

    return mapped_size;
}

static heap_segment_header_t* next_free_kernel_heap_segment(uint64_t size)
{
    heap_segment_header_t* current;
    
    current = kernel_heap.head;
    while (current != NULL)
    {
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    
    if (expand_kernel_heap(size) < size)
        return NULL;
    
    return next_free_kernel_heap_segment(size);
}

static void combine_kernel_heap_forward(heap_segment_header_t* seg)
{
    if (seg->next == NULL || !seg->next->free)
        return;
    if (seg->next == kernel_heap.tail)
        kernel_heap.tail = seg;
    seg->size += seg->next->size + sizeof(heap_segment_header_t);
    seg->next = seg->next->next;
}

static void combine_kernel_heap_backward(heap_segment_header_t* seg)
{
    if (seg->prev != NULL && seg->prev->free)
        combine_kernel_heap_forward(seg->prev);
}

int init_kernel_heap(uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t initial_size)
{
    if (initial_size > ceil_vaddr - start_vaddr || initial_size < sizeof(heap_segment_header_t))
        return -1;
    
    kernel_heap.ceil_vaddr = ceil_vaddr;
    kernel_heap.start_vaddr = start_vaddr;
    kernel_heap.end_vaddr = start_vaddr;
    kernel_heap.tail = NULL;
    if (expand_kernel_heap(initial_size) < initial_size)
        return -1;
    kernel_heap.head = kernel_heap.tail;

    return 0;
}

uint64_t allocate_kernel_heap_memory(uint64_t size)
{
    heap_segment_header_t* seg;
    heap_segment_header_t* new;

    size = ((size < MIN_ALLOC_SIZE) ? MIN_ALLOC_SIZE : size);

    do {
        seg = next_free_kernel_heap_segment(size + sizeof(heap_segment_header_t));
        if (seg == NULL)
            return 0;
    } while (seg->size < size);
    
    new = (heap_segment_header_t*) (seg->data + size);
    new->free = 1;
    new->size = seg->size - size - sizeof(heap_segment_header_t);
    new->prev = seg;
    new->next = seg->next;
    if (new->next != NULL)
        new->next->prev = new;
    new->data = (uint64_t)(new + 1);
    if (seg == kernel_heap.tail)
        kernel_heap.tail = new;

    seg->free = 0;
    seg->size = size;
    seg->next = new;

    return seg->data;
}

void free_kernel_heap_memory(uint64_t addr)
{
    heap_segment_header_t* seg;
    seg = (heap_segment_header_t*) (addr - sizeof(heap_segment_header_t));
    seg->free = 1;
    combine_kernel_heap_forward(seg);
    combine_kernel_heap_backward(seg);
}