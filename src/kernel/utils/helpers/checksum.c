#include "checksum.h"
#include <stdint.h>

#define STRUCT_CHECKSUM(bits) inline void gen_struct_checksum##bits(void* struct_addr, uint64_t size) \
{\
    uint64_t i; \
    uint8_t* ptr; \
    uint##bits##_t* checksum; \
    size -= sizeof(uint##bits##_t); \
    checksum = ((uint##bits##_t*)(((uint64_t) struct_addr) + size)); \
    *checksum = 0; \
    ptr = (uint8_t*) struct_addr; \
    for (i = 0; i < size; i++) \
        *checksum -= ptr[i]; \
}

#define STRUCT_CHECK(bits) inline int check_struct_checksum##bits(void* struct_addr, uint64_t size) \
{\
    uint64_t i; \
    uint8_t* ptr; \
    uint##bits##_t checksum; \
    size -= sizeof(uint##bits##_t); \
    checksum = *((uint##bits##_t*)(((uint64_t) struct_addr) + size)); \
    ptr = (uint8_t*) struct_addr; \
    for (i = 0; i < size; i++) \
        checksum += ptr[i]; \
    return (checksum == 0); \
}

STRUCT_CHECKSUM(64);
STRUCT_CHECKSUM(32);
STRUCT_CHECKSUM(16);
STRUCT_CHECKSUM(8);

STRUCT_CHECK(64);
STRUCT_CHECK(32);
STRUCT_CHECK(16);
STRUCT_CHECK(8);