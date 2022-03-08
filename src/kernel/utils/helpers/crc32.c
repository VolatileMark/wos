#include "crc32.h"

#define CRC32_MAGIC 0xEDB88320

static uint32_t crc32_lookup_table[256];

void fill_crc32_lookup_table(void)
{
    uint8_t index, z;

    index = 0;
    do {
        crc32_lookup_table[index] = (uint32_t) index;
        for (z = 8; z > 0; z--)
            crc32_lookup_table[index] = (crc32_lookup_table[index] & 1) ? ((crc32_lookup_table[index] >> 1) ^ CRC32_MAGIC) : (crc32_lookup_table[index] >> 1);
    } while (++index);
}

uint32_t calculate_crc32(void* p, uint64_t len)
{
    uint32_t crc;
    uint8_t* data;

    crc = 0xFFFFFFFF;
    data = (uint8_t*) p;
    while (len-- != 0)
        crc = crc32_lookup_table[(((uint8_t) crc) ^ *(data++))] ^ (crc >> 8);
    return (~crc);
}