#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#define SIZE_1KB 0x400
#define SIZE_4KB (SIZE_1KB * 4)
#define SIZE_1MB 0x100000

typedef enum
{
    PL0 = 0x00,
    PL1 = 0x01,
    PL2 = 0x02,
    PL3 = 0x03
} privilege_level_t;

#endif