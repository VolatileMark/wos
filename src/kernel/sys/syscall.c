#include "scheduler.h"
#include "../mem/kheap.h"
#include <stdint.h>

#define POP_STACK(T, stack) *((T*) stack++)
#define NUM_SYSCALLS 2
#define INVALID_SYSCALL -128

typedef int (*syscall_handler_t)(process_t* ps, uint64_t* stack_ptr);

extern void switch_to_kernel(cpu_state_t* state);

static int sysexit(process_t* ps, uint64_t* stack)
{
    static int exit_code;
    exit_code = POP_STACK(int, stack);
    
    switch_to_kernel(&ps->user_mode);
    
    ps = get_current_scheduled_process();
    ps->user_mode.regs.rax = exit_code;
    
    terminate_process(ps);
    run_scheduler();
    
    return -1;
}

static int sysexpheap(process_t* ps, uint64_t* stack)
{
    heap_t* heap = POP_STACK(heap_t*, stack);
    uint64_t size = POP_STACK(uint64_t, stack);
    uint64_t* out_size = POP_STACK(uint64_t*, stack);
    *out_size = expand_process_heap(ps, heap, size);
    return 0;
}

static syscall_handler_t handlers[NUM_SYSCALLS] = {
    sysexit,
    sysexpheap
};

int syscall_handler(uint64_t num, uint64_t* stack)
{
    process_t* ps = get_current_scheduled_process();
    if (num < NUM_SYSCALLS)
        return handlers[num](ps, stack);
    return INVALID_SYSCALL;
}