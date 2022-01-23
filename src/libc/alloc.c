#include "alloc.h"
#ifdef __ALLOC_H__

#include "stddef.h"
#include "mem.h"
#include "heap.h"

void* malloc(uint64_t size)
{
    return (void*) allocate_heap_memory(size);
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
    free_heap_memory((uint64_t) ptr);
}

#endif