#include "../syscall.h"

#define trace_exec(msg, ...) trace("EXEC", msg, ##__VA_ARGS__)

static void execve_hook(uint64_t* stack)
{
    process_t* new;
    process_t* old;

    new = get_arg(0, process_t*);
    old = get_arg(1, process_t*);

    scheduler_replace_process(old, new);
    scheduler_run();
}

DEFSYSCALL(execve)
{
    process_t* current;
    process_t* ps;

    current = scheduler_get_current_process();
    ps = process_create_replacement(current, get_arg(0, const char*), get_arg(1, const char**), get_arg(2, const char**));
    if (ps == NULL)
    {
        trace_exec("Failed to create replacement process");
        return -1;
    }
    set_arg(0, ps);
    set_arg(1, current);

    syscall_switch_to_kernel_stack(&execve_hook, stack);

    return -1;
}
