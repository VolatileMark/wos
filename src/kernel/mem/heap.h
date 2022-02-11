#ifndef __KHEAP_H__
#define __KHEAP_H__

#include "../proc/process.h"
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

int init_kernel_heap(uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t inital_size);
uint64_t allocate_kernel_heap_memory(uint64_t size);
void free_kernel_heap_memory(uint64_t addr);

#endif