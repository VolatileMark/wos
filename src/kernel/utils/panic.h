#ifndef __PANIC_H__
#define __PANIC_H__

#include "../sys/cpu/isr.h"

#define READ_REGISTER(reg, var) __asm__ (\
    "mov %%" reg ", %%rax\n\t"\
    "mov %%rax, %0\n\t"\
    : "=m" (var) \
    )

__attribute__((noreturn))
void panic(const interrupt_frame_t* frame, const char* msg, ...);

#endif