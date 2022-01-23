#include "heap.h"
#ifdef __HEAP_H__

#include "stddef.h"
#include "syscall.h"

#define MIN_ALLOC_SIZE sizeof(uint64_t)

static heap_t heap_state;

static uint64_t default_expand_heap(heap_t* heap, uint64_t size)
{
    uint64_t heap_size;
    syscall(SYS_expheap, heap, size, &heap_size);
    return heap_size;
}

int init_heap(expand_heap_func_t expand_heap_func, uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t initial_size, PRIVILEGE_LEVEL privilege_level)
{
    //heap_segment_header_t* first;

    if (initial_size > ceil_vaddr - start_vaddr || initial_size < sizeof(heap_segment_header_t))
        return -1;
    
    heap_state.expand = (expand_heap_func == NULL) ? default_expand_heap : expand_heap_func;
    heap_state.ceil_vaddr = ceil_vaddr;
    heap_state.start_vaddr = start_vaddr;
    heap_state.end_vaddr = start_vaddr;
    heap_state.privilege_level = privilege_level;
    heap_state.tail = NULL;
    if (heap_state.expand(&heap_state, initial_size) < initial_size)
        return -2;
    heap_state.head = heap_state.tail;

    return 0;

    /*
    pages = ceil((double) initial_size / SIZE_4KB);
    paddr = request_pages(pages);
    if (paddr == 0)
        return 0;
    
    heap->ceil_vaddr = ceil_vaddr;
    heap->start_vaddr = start_vaddr;
    heap->end_vaddr = start_vaddr + initial_size;
    heap->privilege_level = privilege_level;

    if (pml4_map_memory(pml4, paddr, heap->start_vaddr, initial_size, PAGE_ACCESS_RW, heap->privilege_level) < initial_size)
    {
        free_pages(paddr, pages);
        return 0;
    }

    first = (heap_segment_header_t*) heap->start_vaddr;
    first->size = initial_size - sizeof(heap_segment_header_t);
    first->free = 1;
    first->data = (uint64_t) (first + 1);
    first->next = NULL;
    first->prev = NULL;

    heap->head = first;
    heap->tail = first;

    return paddr;
    */
}

static heap_segment_header_t* next_free_heap_segment(uint64_t size)
{
    heap_segment_header_t* current = heap_state.head;
    while (current != NULL)
    {
        if (current->free && current->size >= size)
            return current;
        current = current->next;
    }
    if (heap_state.expand(&heap_state, size) < size)
        return NULL;
    return next_free_heap_segment(size);
}

uint64_t allocate_heap_memory(uint64_t size)
{
    heap_segment_header_t* seg;
    heap_segment_header_t* new;

    size = ((size < MIN_ALLOC_SIZE) ? MIN_ALLOC_SIZE : size);

    do {
        seg = next_free_heap_segment(size + sizeof(heap_segment_header_t));
        if (seg == NULL)
            return 0;
    } while (seg->size < size);
    
    new = (heap_segment_header_t*)(seg->data + size);
    new->free = 1;
    new->size = seg->size - size - sizeof(heap_segment_header_t);
    new->prev = seg;
    new->next = seg->next;
    if (new->next != NULL)
        new->next->prev = new;
    new->data = (uint64_t)(new + 1);
    if (seg == heap_state.tail)
        heap_state.tail = new;

    seg->free = 0;
    seg->size = size;
    seg->next = new;

    return seg->data;
}

static void combine_heap_forward(heap_segment_header_t* seg)
{
    if (seg->next == NULL || !seg->next->free)
        return;
    if (seg->next == heap_state.tail)
        heap_state.tail = seg;
    seg->size += seg->next->size + sizeof(heap_segment_header_t);
    seg->next = seg->next->next;
}

static void combine_heap_backward(heap_segment_header_t* seg)
{
    if (seg->prev != NULL && seg->prev->free)
        combine_heap_forward(seg->prev);
}

void free_heap_memory(uint64_t addr)
{
    heap_segment_header_t* seg = (heap_segment_header_t*) (addr - sizeof(heap_segment_header_t));
    seg->free = 1;
    combine_heap_forward(seg);
    combine_heap_backward(seg);
}

heap_t* get_heap(void)
{
    return &heap_state;
}

#endif