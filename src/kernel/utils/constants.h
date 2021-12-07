#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <stdint.h>

#define SIZE_4KB 0x1000

typedef enum 
{
    PL0 = 0x00,
    PL1 = 0x01,
    PL2 = 0x02,
    PL3 = 0x03
} PRIVILEGE_LEVEL;

extern uint64_t _start_paddr;
extern uint64_t _start_vaddr;
extern uint64_t _end_paddr;
extern uint64_t _end_vaddr;

#endif