#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#define SIZE_4KB 0x1000
#define SIZE_1MB 0x100000

#define MIN_MEMORY (SIZE_1MB * 16)

typedef enum 
{
    PL0 = 0x00,
    PL1 = 0x01,
    PL2 = 0x02,
    PL3 = 0x03
} privilege_level_t;

extern uint64_t _start_addr;
extern uint64_t _end_addr;

#endif