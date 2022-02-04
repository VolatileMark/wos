#ifndef __KHEAP_H__
#define __KHEAP_H__

#include "../sys/process.h"
#include <heap.h>
#include <stdint.h>

int init_kernel_heap(uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t inital_size);
uint64_t expand_process_heap(process_t* ps, heap_t* heap, uint64_t size);

#endif