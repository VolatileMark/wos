#ifndef __KRNL_SYSCALL_H__
#define __KRNL_SYSCALL_H__

#include "scheduler.h"
#include "../utils/macros.h"
#include "../utils/log.h"
#include "../sys/mem/paging.h"

#define SYSCALL(name) sys_##name
#define DEFSYSCALL(name) int SYSCALL(name)(uint64_t* stack)
#define NARG(n, T) (*((T*) (stack - n)))

typedef int (*syscall_t)(uint64_t* stack_ptr);

extern void syscall_init(void);
extern void syscall_switch_to_kernel_stack(void* ret, uint64_t* stack);

DEFSYSCALL(exit);

#endif