#include "heap.h"
#ifdef __HEAP_H__

#include "stddef.h"
#include "syscall.h"

#define MIN_ALLOC_SIZE sizeof(uint64_t)

static uint64_t default_expand_heap(heap_t* heap, uint64_t size)
{
    uint64_t heap_size;
    syscall(SYS_expheap, heap, size, &heap_size);
    return heap_size;
}

int init_heap(heap_t* heap, expand_heap_func_t expand_heap_func, uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t initial_size, PRIVILEGE_LEVEL privilege_level)
{
    if (initial_size > ceil_vaddr - start_vaddr || initial_size < sizeof(heap_segment_header_t))
        return -1;
    
    heap->expand = (expand_heap_func == NULL) ? default_expand_heap : expand_heap_func;
    heap->ceil_vaddr = ceil_vaddr;
    heap->start_vaddr = start_vaddr;
    heap->end_vaddr = start_vaddr;
    heap->privilege_level = privilege_level;
    heap->tail = NULL;
    if (heap->expand(heap, initial_size) < initial_size)
        return -2;
    heap->head = heap->tail;

    return 0;
}

static heap_segment_header_t* next_free_heap_segment(heap_t* heap, uint64_t size)
{
    heap_segment_header_t* current = heap->head;
    while (current != NULL)
    {
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    if (heap->expand(heap, size) < size)
        return NULL;
    return next_free_heap_segment(heap, size);
}

uint64_t allocate_heap_memory(heap_t* heap, uint64_t size)
{
    heap_segment_header_t* seg;
    heap_segment_header_t* new;

    size = ((size < MIN_ALLOC_SIZE) ? MIN_ALLOC_SIZE : size);

    do {
        seg = next_free_heap_segment(heap, size + sizeof(heap_segment_header_t));
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
    if (seg == heap->tail)
        heap->tail = new;

    seg->free = 0;
    seg->size = size;
    seg->next = new;

    return seg->data;
}

static void combine_heap_forward(heap_t* heap, heap_segment_header_t* seg)
{
    if (seg->next == NULL || !seg->next->free)
        return;
    if (seg->next == heap->tail)
        heap->tail = seg;
    seg->size += seg->next->size + sizeof(heap_segment_header_t);
    seg->next = seg->next->next;
}

static void combine_heap_backward(heap_t* heap, heap_segment_header_t* seg)
{
    if (seg->prev != NULL && seg->prev->free)
        combine_heap_forward(heap, seg->prev);
}

void free_heap_memory(heap_t* heap, uint64_t addr)
{
    heap_segment_header_t* seg = (heap_segment_header_t*) (addr - sizeof(heap_segment_header_t));
    seg->free = 1;
    combine_heap_forward(heap, seg);
    combine_heap_backward(heap, seg);
}

#endif