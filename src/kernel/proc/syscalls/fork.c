#include "../syscall.h"

#define trace_fork(msg, ...) trace("FORK", msg, ##__VA_ARGS__)

DEFSYSCALL(fork)
{
    process_t* parent;
    process_t* new;
    uint64_t* new_pid;

    new_pid = get_arg(0, uint64_t*);
    *new_pid = scheduler_get_next_pid();

    parent = scheduler_get_current_process();
    new = process_clone(parent, *new_pid);
    if (new == NULL)
    {
        trace_fork("Failed to fork process (pid: %u)", parent->pid);
        return -1;
    }

    new->cpu.regs.rax = 0;
    scheduler_queue_process(new);

    return 0;
}