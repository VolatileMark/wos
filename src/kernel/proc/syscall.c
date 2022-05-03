#include "syscall.h"
#include "scheduler.h"
#include "../sys/mem/heap.h"
#include "../sys/mem/paging.h"
#include "../sys/mem/pfa.h"
#include "../utils/constants.h"
#include "../utils/macros.h"
#include "../utils/alloc.h"
#include "../abi/vm-flags.h"
#include <stdint.h>
#include <stddef.h>
#include <mem.h>
#include <math.h>

#define SYSCALL(name) sys_##name
#define DEFINE_SYSCALL(name) static int SYSCALL(name)(uint64_t* stack)
#define POP_STACK(T) *((T*) stack++)
#define INVALID_SYSCALL -128

typedef int (*syscall_handler_t)(uint64_t* stack_ptr);

extern void switch_to_kernel(cpu_state_t* state, uint64_t args_to_copy);

DEFINE_SYSCALL(exit)
{
    UNUSED(stack);
    switch_to_kernel(&(scheduler_get_current_scheduled_process()->user_mode), 0);
    scheduler_terminate_process(scheduler_get_current_scheduled_process());
    scheduler_run();
    return -1;
}

DEFINE_SYSCALL(execve)
{
    const char* path;
    process_t* new;
    process_t* old;

    path = POP_STACK(const char*);
    old = scheduler_get_current_scheduled_process();
    new = process_create_replacement(path, old);
    if (new == NULL)
        return -1;

    switch_to_kernel(&(scheduler_get_current_scheduled_process()->user_mode), 3);

    scheduler_replace_process(old, new);
    scheduler_run();
    
    return 0;
}

DEFINE_SYSCALL(vm_map)
{
    uint64_t hint, size;
    uint64_t vaddr, paddr;
    page_access_type_t prot;
    int flags, fd;
    long offset;
    uint64_t* window;
    process_t* ps;

    hint = POP_STACK(uint64_t);
    size = POP_STACK(uint64_t);
    prot = POP_STACK(page_access_type_t);
    flags = POP_STACK(int);
    fd = POP_STACK(int);
    offset = POP_STACK(long);
    window = POP_STACK(uint64_t*);
    ps = scheduler_get_current_scheduled_process();
    vaddr = 0;
    
    *window = (uint64_t) -1;

    if 
    (
        hint & 0x0000000000000FFF ||
        size & 0x0000000000000FFF ||
        offset & 0x0000000000000FFF
    )
        return EINVAL;

    if ((flags & MAP_ANONYMOUS) && fd == -1 && offset == 0)
        paddr = pfa_request_pages(ceil((double) size / SIZE_4KB));
    else
    {
        /* TODO: Implement mapping fd */
        paddr = 0;
    }

    if (flags & (MAP_FIXED | MAP_FIXED_NOREPLACE))
    {
        if (flags & MAP_FIXED)
            pml4_unmap_memory(ps->pml4, hint, size);
        if (pml4_map_memory(ps->pml4, paddr, hint, size, prot, PL3) < size)
            return EEXIST;
    }
    else
    {
        if (hint < ps->mapping_start_vaddr)
            hint = ps->mapping_start_vaddr;
        if 
        (
            (flags & MAP_32BIT) &&
            (
                hint > alignd(VADDR_GET(0, 0, 2, 0) - size, SIZE_4KB) ||
                pml4_get_next_vaddr(ps->pml4, hint, size, &vaddr) < size
            )
        )
            return ENOMEM;
        else if (pml4_get_next_vaddr(ps->pml4, hint, size, &vaddr) < size)
            return ENOMEM;
        if (pml4_map_memory(ps->pml4, paddr, vaddr, size, prot, PL3) < size)
            return EEXIST;
    }

    if ((flags & MAP_ANONYMOUS) || !(flags & MAP_UNINITIALIZED))
        memset((void*) vaddr, 0, size);

    *window = vaddr;

    return 0;
}

DEFINE_SYSCALL(vm_unmap)
{
    uint64_t vaddr, size;
    process_t* ps;

    vaddr = POP_STACK(uint64_t);
    size = POP_STACK(uint64_t);
    ps = scheduler_get_current_scheduled_process();

    if (pml4_unmap_memory(ps->pml4, vaddr, size) < size)
        return -1;

    return 0;
}

static syscall_handler_t handlers[] = {
    &SYSCALL(exit),
    &SYSCALL(execve),
    &SYSCALL(vm_map),
    &SYSCALL(vm_unmap)
};

#define NUM_SYSCALLS (sizeof(handlers) / sizeof(uint64_t))

int syscall_handler(uint64_t num, uint64_t* stack)
{
    if (num < NUM_SYSCALLS)
        return handlers[num](stack);
    return INVALID_SYSCALL;
}