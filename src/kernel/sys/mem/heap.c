#include "heap.h"
#include "pfa.h"
#include "paging.h"
#include "../../utils/constants.h"
#include "../../utils/alloc.h"
#include "../../utils/log.h"
#include <stddef.h>
#include <math.h>
#include <mem.h>

#define trace_heap(msg, ...) trace("HEAP", msg, ##__VA_ARGS__)

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

static uint64_t heap_expand(uint64_t size)
{
    uint64_t pages_paddr, pages_count, mapped_size, new_size;
    heap_segment_header_t* new;
    
    new = (heap_segment_header_t*) kernel_heap.end_vaddr;
    
    new_size = alignu(size + sizeof(heap_segment_header_t), SIZE_4KB);
    if (kernel_heap.end_vaddr + new_size > kernel_heap.ceil_vaddr)
        return 0;
    
    pages_count = ceil((double) new_size / SIZE_4KB);
    pages_paddr = pfa_request_pages(pages_count);
    if (pages_paddr == 0)
        return 0;
    
    mapped_size = kernel_map_memory(pages_paddr, kernel_heap.end_vaddr, new_size, PAGE_ACCESS_WX, PL0);
    if (mapped_size < new_size)
    {
        pfa_free_pages(pages_paddr, pages_count);
        return 0;
    }

    kernel_heap.end_vaddr += mapped_size;

    new->free = 1;
    new->size = new_size - sizeof(heap_segment_header_t);
    new->data = (uint64_t)(new + 1);
    new->prev = kernel_heap.tail;
    new->next = NULL;
    if (kernel_heap.tail != NULL)
    {
        new->next = kernel_heap.tail->next;
        kernel_heap.tail->next = new;
    }
    kernel_heap.tail = new;

    return mapped_size;
}

static heap_segment_header_t* heap_next_free_segment(uint64_t size)
{
    heap_segment_header_t* current;
    
    current = kernel_heap.head;
    while (current != NULL)
    {
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    
    if (heap_expand(size) < size)
        return NULL;
    
    return heap_next_free_segment(size);
}

static void heap_combine_forward(heap_segment_header_t* seg)
{
    if (seg->next == NULL || !seg->next->free)
        return;
    if (seg->next == kernel_heap.tail)
        kernel_heap.tail = seg;
    seg->size += seg->next->size + sizeof(heap_segment_header_t);
    seg->next = seg->next->next;
}

static void heap_combine_backward(heap_segment_header_t* seg)
{
    if (seg->prev != NULL && seg->prev->free)
        heap_combine_forward(seg->prev);
}

int heap_init(uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t initial_size)
{
    if (initial_size > ceil_vaddr - start_vaddr || initial_size < sizeof(heap_segment_header_t))
    {
        trace_heap("Invalid initial size");
        return -1;
    }
    
    kernel_heap.ceil_vaddr = ceil_vaddr;
    kernel_heap.start_vaddr = start_vaddr;
    kernel_heap.end_vaddr = start_vaddr;
    kernel_heap.tail = NULL;
    if (heap_expand(initial_size) < initial_size)
    {
        trace_heap("Failed to allocate initial memory");
        return -1;
    }
    kernel_heap.head = kernel_heap.tail;

    info("Kernel heap initialized at %p with size %u kB", kernel_heap.start_vaddr, initial_size >> 10);

    return 0;
}

heap_segment_header_t* heap_allocate_memory(uint64_t size)
{
    heap_segment_header_t* seg;
    heap_segment_header_t* new;

    size = ((size < MIN_ALLOC_SIZE) ? MIN_ALLOC_SIZE : size);

    do {
        seg = heap_next_free_segment(size + sizeof(heap_segment_header_t));
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

    return seg;
}

void heap_free_memory(heap_segment_header_t* seg)
{
    seg->free = 1;
    heap_combine_forward(seg);
    heap_combine_backward(seg);
}