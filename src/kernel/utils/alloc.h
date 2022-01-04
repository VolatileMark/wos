#ifndef __ALLOC_H__
#define __ALLOC_H__

#include "../sys/process.h"
#include <stdint.h>

void* kmalloc(uint64_t size);
void* kcalloc(uint64_t n, uint64_t size);
void* krealloc(void* src, uint64_t new_size);
void kfree(void* ptr);

void* pmalloc(uint64_t size, process_t* ps);
void* pcalloc(uint64_t n, uint64_t size, process_t* ps);
void* prealloc(void* src, uint64_t new_size, process_t* ps);
void pfree(void* ptr, process_t* ps);


#endif