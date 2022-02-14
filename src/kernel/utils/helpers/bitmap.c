#include "bitmap.h"

uint8_t bitmap_set(bitmap_t* bitmap, uint64_t index, uint8_t value)
{
    uint64_t byte_index;
    uint8_t bit_index, bit_mask;
    if (index > bitmap->size * 8)
        return 0;
    byte_index = index / 8;
    bit_index = index % 8;
    bit_mask = 0b10000000 >> bit_index;
    bitmap->buffer[byte_index] &= ~bit_mask;
    if (value)
        bitmap->buffer[byte_index] |= bit_mask;
    return 1;
}

uint8_t bitmap_get(bitmap_t* bitmap, uint64_t index)
{
    uint64_t byte_index;
    uint8_t bit_index, bit_mask;
    if (index > bitmap->size * 8)
        return 0;
    byte_index = index / 8;
    bit_index = index % 8;
    bit_mask = 0b10000000 >> bit_index;
    return ((bitmap->buffer[byte_index] & bit_mask) > 0);
}