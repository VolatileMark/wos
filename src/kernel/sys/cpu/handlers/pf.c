#include "../handlers.h"
#include "../gdt.h"
#include "../../../proc/scheduler.h"
#include <stddef.h>

void handler_pf(const interrupt_frame_t* int_frame) 
{
    uint64_t fault_address, size;
    process_t* ps;

    READ_REGISTER("cr2", fault_address);

    ps = scheduler_get_current_process();
    if (ps == NULL || int_frame->stack_state.cs != gdt_get_user_cs())
        panic(int_frame, "Page fault occurred in kernel context. Fault address %p", fault_address);
    
    if 
    (
        fault_address < ps->user_stack_vaddr &&
        (size = ((ps->user_stack_vaddr - fault_address) + ps->user_stack_size)) < PROC_USER_STACK_MAX_SIZE
    )
    {
        if (process_grow_user_stack(ps, size))
            return;
        return;
    }
}