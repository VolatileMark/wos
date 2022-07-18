#ifndef __ALLOC_H__
#define __ALLOC_H__

#include <stdint.h>

void* malloc(uint64_t size);
void* aligned_alloc(uint64_t align, uint64_t size);
void* calloc(uint64_t n, uint64_t size);
void* realloc(void* src, uint64_t new_size);
void free(void* ptr);

#endif
