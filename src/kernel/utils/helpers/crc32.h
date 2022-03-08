#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>

#define CRC32_VALID 0x2144DF1C

void fill_crc32_lookup_table(void);
uint32_t calculate_crc32(void* data, uint64_t len);

#endif