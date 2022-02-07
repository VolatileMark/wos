#if !defined(__LIBC_SYSCALL_H__) && !defined(LIBC_UTILS_ONLY)
#define __LIBC_SYSCALL_H__

#include "stdint.h"

#define SYS_exit 0
#define SYS_execve 1
#define SYS_heap_expand 2
#define SYS_spp_expose 3
#define SYS_spp_request 4

extern int syscall(uint64_t code, ...);

#endif