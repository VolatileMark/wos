#ifndef __CRC32_H__
#define __CRC32_H__

#include <stdint.h>

void crc32_fill_lookup_table(void);
uint32_t crc32_calculate(void* data, uint64_t len);

#endif