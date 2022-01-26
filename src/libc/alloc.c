#include "alloc.h"
#ifdef __ALLOC_H__

#include "stddef.h"
#include "mem.h"
#include "heap.h"

static heap_t heap_state;

int init_alloc(expand_heap_func_t expand_heap_func, uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t initial_size, PRIVILEGE_LEVEL privilege_level)
{
    return init_heap(&heap_state, expand_heap_func, start_vaddr, ceil_vaddr, initial_size, privilege_level);
}

void* malloc(uint64_t size)
{
    return (void*) allocate_heap_memory(&heap_state, size);
}

void* calloc(uint64_t n, uint64_t size)
{
    size = n * size;
    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void* realloc(void* src, uint64_t new_size)
{
    heap_segment_header_t* hdr = ((heap_segment_header_t*) src) - 1;
    void* new_ptr = malloc(new_size);
    memcpy(new_ptr, src, (hdr->size < new_size) ? hdr->size : new_size);
    free(src);
    return new_ptr;
}

void free(void* ptr)
{
    free_heap_memory(&heap_state, (uint64_t) ptr);
}

#endif