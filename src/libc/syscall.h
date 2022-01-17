#ifndef __LIBC_SYSCALL_H__
#define __LIBC_SYSCALL_H__

#include "stdint.h"

#define SYS_exit 0

extern void syscall(uint64_t code, ...);

#endif