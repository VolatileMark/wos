#ifndef __CHECKSUM_H__
#define __CHECKSUM_H__

#include <stdint.h>

#define DEF_STRUCT_CHECKSUM(bits) void gen_struct_checksum##bits(void* struct_addr, uint64_t size)
#define DEF_CHECKSUM(bits) int checksum##bits(void* struct_addr, uint64_t size)

DEF_STRUCT_CHECKSUM(64);
DEF_STRUCT_CHECKSUM(32);
DEF_STRUCT_CHECKSUM(16);
DEF_STRUCT_CHECKSUM(8);

DEF_CHECKSUM(64);
DEF_CHECKSUM(32);
DEF_CHECKSUM(16);
DEF_CHECKSUM(8);

#endif
