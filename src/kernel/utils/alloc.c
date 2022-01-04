#include "alloc.h"
#include "../mem/heap.h"
#include <stddef.h>
#include <mem.h>

void* kmalloc(uint64_t size)
{
    return (void*) allocate_heap_memory(size);
}

void* kcalloc(uint64_t n, uint64_t size)
{
    size = n * size;
    void* ptr = kmalloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void* krealloc(void* src, uint64_t new_size)
{
    heap_segment_header_t* hdr = ((heap_segment_header_t*) src) - 1;
    void* new_ptr = kmalloc(new_size);
    memcpy(new_ptr, src, (hdr->size < new_size) ? hdr->size : new_size);
    kfree(src);
    return new_ptr;
}

void kfree(void* ptr)
{
    free_heap_memory((uint64_t) ptr);
}