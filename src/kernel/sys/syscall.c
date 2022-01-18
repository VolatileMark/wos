#include "scheduler.h"
#include <stdint.h>
#include <stdarg.h>
#include <syscall.h>

#define NUM_SYSCALLS 1
#define INVALID_SYSCALL -128

typedef int (*syscall_handler_t)(process_t* ps, va_list ap);

extern void switch_to_kernel(cpu_state_t* state);

static int sysexit(process_t* ps, va_list ap)
{
    static int exit_code;
    
    exit_code = va_arg(ap, int);
    va_end(ap);
    
    switch_to_kernel(&ps->user_mode);
    
    ps = get_current_scheduled_process();
    ps->user_mode.regs.rax = exit_code;
    
    terminate_process(ps);
    run_scheduler();
    
    return -1;
}

static syscall_handler_t handlers[NUM_SYSCALLS] = {
    sysexit
};

int syscall_handler(uint64_t num, ...)
{
    int return_code = INVALID_SYSCALL;
    process_t* ps = get_current_scheduled_process();
    va_list ap;
    va_start(ap, num);
    if (num < NUM_SYSCALLS)
        return_code = handlers[num](ps, ap);
    va_end(ap);
    return return_code;
}