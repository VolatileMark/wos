#if !defined(__LIBC_SYSCALL_H__) && !defined(LIBC_UTILS_ONLY)
#define __LIBC_SYSCALL_H__

#include "stdint.h"

#define SYS_exit 0
#define SYS_expheap 2

extern int syscall(uint64_t code, ...);

#endif