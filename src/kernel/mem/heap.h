#ifndef __HEAP_H__
#define __HEAP_H__

#include <stdint.h>

struct heap_segment_header 
{
    struct heap_segment_header* next;
    struct heap_segment_header* prev;
    uint64_t data;
    uint64_t size : 63;
    uint64_t free : 1;
} __attribute__((packed));
typedef struct heap_segment_header heap_segment_header_t;

int init_heap(uint64_t addr, uint64_t pages);
uint64_t allocate_heap_memory(uint64_t size);
void free_heap_memory(uint64_t addr);

#endif