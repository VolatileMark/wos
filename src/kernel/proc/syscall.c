#include "../utils/macros.h"
#include <stdint.h>

#define SYSCALL(name) sys_##name
#define DEFSYSCALL(name) int SYSCALL(name)(uint64_t* stack)
#define NARG(n, T) ((T) stack[n])

typedef int (*syscall_t)(uint64_t* stack_ptr);

DEFSYSCALL(null)
{
    UNUSED(stack);
    return 0;
}

static syscall_t syscalls[] = {
    &SYSCALL(null)
};

#define NUM_SYSCALLS (sizeof(syscalls) / sizeof(uint64_t))

int syscall_handler(uint64_t num, uint64_t* stack)
{
    if (num > NUM_SYSCALLS)
        return -1;
    return syscalls[num](stack);
}