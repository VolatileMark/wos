#ifndef __ALLOC_H__
#define __ALLOC_H__

#include <stdint.h>

void* kmalloc(uint64_t size);
void* kcalloc(uint64_t n, uint64_t size);
void* krealloc(void* src, uint64_t new_size);
void kfree(void* ptr);

#endif