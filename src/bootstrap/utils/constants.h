#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#define SIZE_4KB 0x1000
#define MIN_MEMORY 0x1600000
#define INITRD_RELOC_ADDR 0x1200000

typedef enum 
{
    PL0 = 0x00,
    PL1 = 0x01,
    PL2 = 0x02,
    PL3 = 0x03
} PRIVILEGE_LEVEL;

extern uint64_t _start_addr;
extern uint64_t _end_addr;

#endif