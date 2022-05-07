#include "alloc.h"
#include "checksum.h"
#include "../sys/mem/heap.h"
#include <stddef.h>
#include <math.h>
#include <mem.h>

void* malloc(uint64_t size)
{
    return ((void*) (heap_allocate_memory(size) + 1));
}

void* aligned_alloc(uint64_t align, uint64_t size)
{
    return ((void*) (heap_allocate_aligned_memory(align, size) + 1));
}

void* calloc(uint64_t n, uint64_t size)
{
    void* ptr;
    size = n * size;
    ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void* realloc(void* src, uint64_t new_size)
{
    heap_segment_header_t* hdr;
    void* new_ptr;
    hdr = ((heap_segment_header_t*) src) - 1;
    new_ptr = malloc(new_size);
    memcpy(new_ptr, src, (hdr->size < new_size) ? hdr->size : new_size);
    free(src);
    return new_ptr;
}

void free(void* ptr)
{
    heap_segment_header_t* seg;
    seg = ((heap_segment_header_t*) ptr) - 1;
    heap_free_memory(seg);
}