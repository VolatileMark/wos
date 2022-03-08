#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>

void fill_crc32_lookup_table(void);
uint32_t calculate_crc32(void* data, uint64_t len);

#endif