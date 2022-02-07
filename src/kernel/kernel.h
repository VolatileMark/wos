#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "mem/paging.h"
#include <stdint.h>

#define KERNEL_STACK_SIZE SIZE_4KB
#define KERNEL_HEAP_START_ADDR VADDR_GET(384, 0, 0, 0)
#define KERNEL_HEAP_CEIL_ADDR VADDR_GET(511, 509, 511, 511)

void launch_init();

extern uint64_t _start_paddr;
extern uint64_t _start_vaddr;
extern uint64_t _end_paddr;
extern uint64_t _end_vaddr;

#endif