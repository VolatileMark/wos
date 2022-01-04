#ifndef __HEAP_H__
#define __HEAP_H__

#include <stdint.h>

struct process;

struct heap_segment_header 
{
    struct heap_segment_header* next;
    struct heap_segment_header* prev;
    uint64_t data;
    uint64_t size : 63;
    uint64_t free : 1;
} __attribute__((packed));
typedef struct heap_segment_header heap_segment_header_t;

uint64_t init_kernel_heap(uint64_t start_addr, uint64_t end_addr, uint64_t initial_pages);
uint64_t allocate_kernel_heap_memory(uint64_t size);
void free_kernel_heap_memory(uint64_t addr);

#include "../sys/process.h"

uint64_t init_process_heap(uint64_t start_addr, uint64_t end_addr, uint64_t initial_pages, struct process* ps);
uint64_t allocate_process_heap_memory(uint64_t size, struct process* ps);
void free_process_heap_memory(uint64_t addr, struct process* ps);

#endif