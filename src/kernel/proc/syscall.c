#include "syscall.h"

static syscall_t syscalls[] = {
    &SYSCALL(exit),
    &SYSCALL(execve),
    &SYSCALL(fork)
};

#define NUM_SYSCALLS (sizeof(syscalls) / sizeof(uint64_t))

int syscall_handler(uint64_t num, uint64_t* stack)
{
    if (num > NUM_SYSCALLS)
        return -1;
    return syscalls[num](stack);
}