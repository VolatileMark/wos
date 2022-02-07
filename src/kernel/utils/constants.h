#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#define SIZE_4KB 0x1000

typedef enum PRIVILEGE_LEVEL_ENUM
{
    PL0 = 0x00,
    PL1 = 0x01,
    PL2 = 0x02,
    PL3 = 0x03
} PRIVILEGE_LEVEL;

#endif