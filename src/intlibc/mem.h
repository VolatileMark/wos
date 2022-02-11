#ifndef __MEM_H__
#define __MEM_H__

#include "stdint.h"

void memset(void* ptr, uint8_t c, uint64_t size);
void memcpy(void* dest, const void* src, uint64_t size);
uint64_t alignu(uint64_t address, uint64_t size);
uint64_t alignd(uint64_t address, uint64_t size);

#endif