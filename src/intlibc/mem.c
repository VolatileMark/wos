#include "mem.h"

uint64_t alignu(uint64_t size, uint64_t align)
{
    uint64_t blocks = size % align;
    if (blocks == 0)
        return size;
    return (size + (align - blocks));
}

uint64_t alignd(uint64_t size, uint64_t align)
{
    return (size - (size % align));
}

void memset(void* ptr, uint8_t c, uint64_t size)
{
    uint8_t* cptr = (uint8_t*) ptr;
    for (; size > 0; size--)
        *cptr ++ = c;
}

void memcpy(void* dest, const void* src, uint64_t size)
{
    uint8_t* tmpSrc = (uint8_t*) src;
    uint8_t* tmpDest = (uint8_t*) dest;
    for (; size > 0; size--)
        *tmpDest++ = *tmpSrc++;
}