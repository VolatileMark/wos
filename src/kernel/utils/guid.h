#ifndef __GUID_H__
#define __GUID_H__

#include <stdint.h>

struct guid
{
    uint32_t d1;
    uint16_t d2;
    uint16_t d3;
    uint8_t d4[8];
} __attribute__((packed));
typedef struct guid guid_t;

int guidcmp(guid_t* g1, guid_t* g2);

#endif