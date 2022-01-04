#include "alloc.h"
#include "../mem/heap.h"
#include <stddef.h>
#include <mem.h>

void* kmalloc(uint64_t size)
{
    return (void*) allocate_kernel_heap_memory(size);
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
    free_kernel_heap_memory((uint64_t) ptr);
}

void* pmalloc(uint64_t size, process_t* ps)
{
    return (void*) allocate_process_heap_memory(size, ps);
}

void* pcalloc(uint64_t n, uint64_t size, process_t* ps)
{
    size = n * size;
    void* ptr = pmalloc(size, ps);
    memset(ptr, 0, size);
    return ptr;
}

void* prealloc(void* src, uint64_t new_size, process_t* ps)
{
    heap_segment_header_t* hdr = ((heap_segment_header_t*) src) - 1;
    void* new_ptr = pmalloc(new_size, ps);
    memcpy(new_ptr, src, (hdr->size < new_size) ? hdr->size : new_size);
    pfree(src, ps);
    return new_ptr;
}

void pfree(void* ptr, process_t* ps)
{
    free_process_heap_memory((uint64_t) ptr, ps);
}