#include "../syscall.h"

#define trace_exit(msg, ...) trace("EXIT", msg, ##__VA_ARGS__)
#define EXIT_STATUS_OK 0

static void exit_hook(uint64_t* stack)
{
    int status;
    process_t* ps;

    status = NARG(0, int);
    ps = scheduler_get_current_process();
    pml4_load(kernel_get_pml4_paddr());
    if (scheduler_terminate_process(ps))
    {
        trace_exit("Failed to terminate process");
        HALT();
    }
    if (status != EXIT_STATUS_OK)
        trace_exit("Process %u terminated with exit error code: %d", ps->pid, (long) status);
    
    scheduler_run();
}

DEFSYSCALL(exit)
{
    syscall_switch_to_kernel_stack(&exit_hook, stack);
    return -1;
}