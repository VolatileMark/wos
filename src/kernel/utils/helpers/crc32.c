#include "crc32.h"

#define CRC32_MAGIC 0x04C11DB7

static uint32_t crc32_lookup_table[256];

static uint8_t byte_bit_reflect(uint8_t byte)
{
    uint8_t out, i;
    out = 0;
    for (i = 8; i > 0; i--)
    {
        out |= (byte & 1) << (i - 1);
        byte >>= 1;
    }
    return out;
}

void fill_crc32_lookup_table(void)
{
    uint8_t index, z;
    uint32_t current;

    index = 0;
    do {
        current = ((uint32_t) byte_bit_reflect(index)) << 23;
        for (z = 8; z > 0; z--)
        {
            if (current & (1 << 31))
            {
                current <<= 1;
                current ^= CRC32_MAGIC;
            }
            else 
                current <<= 1;
        }
        crc32_lookup_table[index] = current;
    } while (++index);
}

uint32_t calculate_crc32(void* p, uint64_t len)
{
    uint32_t crc;
    uint8_t* data;

    crc = 0xFFFFFFFF;
    data = (uint8_t*) p;
    for (; len > 0; len--)
        crc = crc32_lookup_table[(((uint8_t) crc) ^ *data++)] ^ (crc >> 8);
    return crc;
}