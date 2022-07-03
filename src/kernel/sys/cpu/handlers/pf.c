#include "../handlers.h"
#include "../gdt.h"
#include "../../../proc/scheduler.h"
#include "../../../utils/log.h"
#include <stddef.h>

#define trace_pf(msg, ...) trace("PGFT", msg, ##__VA_ARGS__)

void handler_pf(const interrupt_frame_t* int_frame) 
{
    uint64_t fault_address, size;
    process_t* ps;

    READ_REGISTER("cr2", fault_address);

    ps = scheduler_get_current_process();
    if (ps == NULL)
        panic(int_frame, "Page fault occurred in kernel context. Fault address %p", fault_address);
    
    if 
    (
        fault_address < ps->user_stack.ceil &&
        (size = ps->user_stack.floor - fault_address) < PROC_MAX_STACK_SIZE
    )
    {
        if (process_grow_stack(ps, &ps->user_stack, size))
        {
            trace_pf("Failed to expand user stack (pid: %u)", ps->pid);
            scheduler_terminate_process(ps);
            return;
        }
        return;
    }
    else if
    (
        fault_address < ps->kernel_stack.ceil &&
        (size = ps->kernel_stack.floor - fault_address) < PROC_MAX_STACK_SIZE
    )
    {
        if (process_grow_stack(ps, &ps->kernel_stack, size))
        {
            trace_pf("Failed to expand kernel stack (pid: %u)", ps->pid);
            scheduler_terminate_process(ps);
            return;
        }
        return;
    }
}