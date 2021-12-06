#include "bitmap.h"

uint8_t bitmap_set(bitmap_t* bitmap, uint64_t index, uint8_t value)
{
    if (index > bitmap->size * 8)
        return 0;
    uint64_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    uint8_t bitMask = 0b10000000 >> bitIndex;
    bitmap->buffer[byteIndex] &= ~bitMask;
    if (value)
        bitmap->buffer[byteIndex] |= bitMask;
    return 1;
}

uint8_t bitmap_get(bitmap_t* bitmap, uint64_t index)
{
    if (index > bitmap->size * 8)
        return 0;
    uint64_t byteIndex = index / 8;
    uint8_t bitIndex = index % 8;
    uint8_t bitMask = 0b10000000 >> bitIndex;
    return ((bitmap->buffer[byteIndex] & bitMask) > 0);
}