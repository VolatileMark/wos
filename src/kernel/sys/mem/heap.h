#ifndef __KHEAP_H__
#define __KHEAP_H__

#include <stdint.h>

struct heap_segment_header 
{
    struct heap_segment_header* next;
    struct heap_segment_header* prev;
    uint64_t size;
    uint64_t free : 1;
    uint64_t reserved : 63;
} __attribute__((packed));
typedef struct heap_segment_header heap_segment_header_t;

int heap_init(uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t inital_size);
heap_segment_header_t* heap_allocate_memory(uint64_t size);
heap_segment_header_t* heap_allocate_aligned_memory(uint64_t alignment, uint64_t size);
void heap_free_memory(heap_segment_header_t* seg);

#endif
