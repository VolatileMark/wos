#ifndef __BITMAP_H__
#define __BITMAP_H__ 1

#include <stdint.h>

typedef struct
{
    uint64_t size;
    uint8_t* buffer;
} bitmap_t;

uint8_t bitmap_set(bitmap_t* bitmap, uint64_t index, uint8_t value);
uint8_t bitmap_get(bitmap_t* bitmap, uint64_t index);

#endif