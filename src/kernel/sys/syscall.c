#include "scheduler.h"
#include "ipc/spp.h"
#include "../mem/kheap.h"
#include "../utils/macros.h"
#include <stdint.h>
#include <stddef.h>

#define POP_STACK(T, stack) *((T*) stack++)
#define NUM_SYSCALLS 4
#define INVALID_SYSCALL -128

typedef int (*syscall_handler_t)(uint64_t* stack_ptr);

extern void switch_to_kernel(cpu_state_t* state);

static int sys_exit(uint64_t* stack)
{
    UNUSED(stack);
    switch_to_kernel(&(get_current_scheduled_process()->user_mode));
    terminate_process(get_current_scheduled_process());
    run_scheduler();
    return -1;
}

static int sys_execve(uint64_t* stack)
{
    const char* path;
    process_t* current;
    process_t* new;

    path = POP_STACK(const char*, stack);
    current = get_current_scheduled_process();
    new = create_replacement_process(current, NULL);

    UNUSED(path);
    UNUSED(current);
    UNUSED(new);

    return 0;
}

static int sys_expheap(uint64_t* stack)
{
    heap_t* heap = POP_STACK(heap_t*, stack);
    uint64_t size = POP_STACK(uint64_t, stack);
    uint64_t* out_size = POP_STACK(uint64_t*, stack);
    *out_size = expand_process_heap(get_current_scheduled_process(), heap, size);
    return 0;
}

static int sys_sppreg(uint64_t* stack)
{
    uint64_t fn_addr = POP_STACK(uint64_t, stack);
    const char* fn_name = POP_STACK(const char*, stack);
    return expose_function(get_current_scheduled_process(), fn_addr, fn_name);
}

static syscall_handler_t handlers[NUM_SYSCALLS] = {
    sys_exit,
    sys_execve,
    sys_expheap,
    sys_sppreg
};

int syscall_handler(uint64_t num, uint64_t* stack)
{
    if (num < NUM_SYSCALLS)
        return handlers[num](stack);
    return INVALID_SYSCALL;
}