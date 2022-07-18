#include "checksum.h"

#define STRUCT_CHECKSUM(bits) inline void gen_struct_checksum##bits(void* addr, uint64_t size) \
{\
    uint64_t i; \
    uint8_t* ptr; \
    uint##bits##_t* checksum; \
    size -= sizeof(uint##bits##_t); \
    checksum = ((uint##bits##_t*)(((uint64_t) addr) + size)); \
    *checksum = 0; \
    ptr = (uint8_t*) addr; \
    for (i = 0; i < size; i++) \
        *checksum -= ptr[i]; \
}

#define CHECKSUM(bits) inline int checksum##bits(void* addr, uint64_t size) \
{\
    uint64_t i; \
    uint8_t* ptr; \
    uint##bits##_t checksum; \
    size -= sizeof(uint##bits##_t); \
    checksum = *((uint##bits##_t*)(((uint64_t) addr) + size)); \
    ptr = (uint8_t*) addr; \
    for (i = 0; i < size; i++) \
        checksum += ptr[i]; \
    return (checksum == 0); \
}

STRUCT_CHECKSUM(64);
STRUCT_CHECKSUM(32);
STRUCT_CHECKSUM(16);
STRUCT_CHECKSUM(8);

CHECKSUM(64);
CHECKSUM(32);
CHECKSUM(16);
CHECKSUM(8);
