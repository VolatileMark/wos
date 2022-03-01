#ifndef __CHECKSUM_H__
#define __CHECKSUM_H__

#include <stdint.h>

#define DEF_STRUCT_CHECKSUM(bits) void gen_struct_checksum##bits(void* struct_addr, uint64_t size)
#define DEF_STRUCT_CHECK(bits) int check_struct_checksum##bits(void* struct_addr, uint64_t size)

DEF_STRUCT_CHECKSUM(64);
DEF_STRUCT_CHECKSUM(32);
DEF_STRUCT_CHECKSUM(16);
DEF_STRUCT_CHECKSUM(8);

DEF_STRUCT_CHECK(64);
DEF_STRUCT_CHECK(32);
DEF_STRUCT_CHECK(16);
DEF_STRUCT_CHECK(8);

#endif