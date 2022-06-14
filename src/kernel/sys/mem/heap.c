#include "heap.h"
#include "pfa.h"
#include "paging.h"
#include "../../utils/log.h"
#include "../../utils/panic.h"
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
    
    mapped_size = paging_map_memory(pages_paddr, kernel_heap.end_vaddr, new_size, PAGE_ACCESS_RW, PL0);
    if (mapped_size < new_size)
    {
        pfa_free_pages(pages_paddr, pages_count);
        return 0;
    }

    kernel_heap.end_vaddr += mapped_size;

    new->free = 1;
    new->size = new_size - sizeof(heap_segment_header_t);
    new->prev = kernel_heap.tail;
    new->next = NULL;
    if (kernel_heap.tail != NULL)
    {
        new->next = kernel_heap.tail->next;
        kernel_heap.tail->next = new;
    }
    kernel_heap.tail = new;
    heap_combine_backward(new);

    return mapped_size;
}

static heap_segment_header_t* heap_next_free_segment(uint64_t size)
{
    heap_segment_header_t* current;
    uint64_t heap_size;

    current = kernel_heap.head;
    heap_size = kernel_heap.end_vaddr - kernel_heap.start_vaddr;
    while (current != NULL)
    {
        if 
        (
            (current->next != NULL && (((uint64_t) current->next) < kernel_heap.start_vaddr || ((uint64_t) current->next) > kernel_heap.end_vaddr)) ||
            (current->prev != NULL && (((uint64_t) current->prev) < kernel_heap.start_vaddr || ((uint64_t) current->prev) > kernel_heap.end_vaddr)) ||
            (current->size > heap_size || current->size < MIN_ALLOC_SIZE)
        )
            panic
            (
                NULL, 
                "Kernel heap corruption detected.\n\t\t  Corrupted header: %p"
                "\n\t\t\t->next = %p\n\t\t\t->prev = %p\n\t\t\t->size = %u\n\t\t\t->free = %u",
                current, current->next, current->prev, current->size, current->free
            );
        
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    
    if (heap_expand(size) < size)
        return NULL;
    
    return heap_next_free_segment(size);
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
            return NULL;
    } while (seg->size < size);
    
    new = (heap_segment_header_t*) (((uint64_t)(seg + 1)) + size);
    new->free = 1;
    new->size = seg->size - size - sizeof(heap_segment_header_t);
    new->prev = seg;
    new->next = seg->next;
    if (new->next != NULL)
        new->next->prev = new;
    if (seg == kernel_heap.tail)
        kernel_heap.tail = new;

    seg->free = 0;
    seg->size = size;
    seg->next = new;

    return seg;
}

heap_segment_header_t* heap_allocate_aligned_memory(uint64_t alignment, uint64_t size)
{
    heap_segment_header_t* seg;
    heap_segment_header_t* aligned;
    uint64_t aligned_data_addr, data_addr;

    seg = heap_allocate_memory(size + alignment + (sizeof(heap_segment_header_t) * 2));
    if (seg == NULL)
        return NULL;
    
    data_addr = (uint64_t) (seg + 1);
    aligned_data_addr = alignu(data_addr, alignment);
    if ((aligned_data_addr - data_addr) < sizeof(heap_segment_header_t))
        aligned_data_addr = alignu(data_addr + sizeof(heap_segment_header_t), alignment);
    
    aligned = ((heap_segment_header_t*) aligned_data_addr) - 1;
    aligned->free = 0;
    aligned->size = (((uint64_t) (seg + 1)) + seg->size) - aligned_data_addr;
    aligned->next = NULL;
    aligned->prev = seg;
    
    return aligned;
}

void heap_free_memory(heap_segment_header_t* seg)
{
    if (seg->next == NULL && seg != kernel_heap.tail)
        seg = seg->prev;
    seg->free = 1;
    heap_combine_forward(seg);
    heap_combine_backward(seg);
}