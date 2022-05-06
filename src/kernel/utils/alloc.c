#include "alloc.h"
#include "checksum.h"
#include "../sys/mem/heap.h"
#include <stddef.h>
#include <mem.h>

#define ALIGN_HEADER_MAGIC 0x414C474E

struct align_header
{
    uint64_t offset;
    uint32_t magic;
    uint32_t checksum;
} __attribute__((packed));
typedef struct align_header align_header_t;

void* malloc(uint64_t size)
{
    return ((void*) heap_allocate_memory(size)->data);
}

void* aligned_alloc(uint64_t align, uint64_t size)
{
    heap_segment_header_t* seg;
    align_header_t* alignhd;
    uint64_t data_ptr;
    size += align;
    seg = heap_allocate_memory(size);
    data_ptr = alignu(seg->data, align);
    alignhd = (align_header_t*)(data_ptr - sizeof(align_header_t));
    alignhd->offset = (data_ptr - ((uint64_t) seg));
    alignhd->magic = ALIGN_HEADER_MAGIC;
    gen_struct_checksum32(alignhd, sizeof(align_header_t));
    return ((void*) data_ptr);
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
    align_header_t* alignhd;
    alignhd = ((align_header_t*) ptr) - 1;
    if 
    (
        alignhd->magic == ALIGN_HEADER_MAGIC &&
        checksum32(alignhd, sizeof(align_header_t))
    )
        seg = (heap_segment_header_t*) (((uint64_t) ptr) - alignhd->offset);
    else
        seg = ((heap_segment_header_t*) ptr) - 1;
    heap_free_memory(seg);
}