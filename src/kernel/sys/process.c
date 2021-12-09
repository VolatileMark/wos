#include "process.h"
#include "../cpu/gdt.h"
#include "../utils/alloc.h"
#include "../utils/constants.h"
#include "../mem/pfa.h"
#include "../mem/paging.h"
#include <stddef.h>
#include <mem.h>
#include "../utils/elf.h"

#define PROC_DEFAULT_STACK_PAGES 1
#define PROC_DEFAULT_STACK_VADDR (((uint64_t) &_start_vaddr) - SIZE_4KB)
#define PROC_DEFAULT_RSP (((uint64_t) &_start_vaddr) - sizeof(uint64_t))
#define PROC_DEFAULT_RFLAGS 0x202 /* Only interrupts enabled */

static void init_process(process_t* ps, uint64_t pid)
{
    ps->pid = pid;
    ps->parent_pid = 0;
    ps->pml4 = NULL;
    ps->pml4_paddr = 0;
    ps->kernel_stack_start_vaddr = 0;
    ps->code_start_vaddr = 0;
    ps->stack_start_vaddr = PROC_DEFAULT_STACK_VADDR;
    ps->kernel_stack_segments.head = NULL;
    ps->kernel_stack_segments.tail = NULL;
    ps->code_segments.head = NULL;
    ps->code_segments.tail = NULL;
    ps->stack_segments.head = NULL;
    ps->stack_segments.tail = NULL;

    memset(ps->fds, 0, PROCESS_MAX_FDS * sizeof(file_descriptor_t));
    memset(&ps->user_mode, 0, sizeof(cpu_state_t));
    memset(&ps->current, 0, sizeof(cpu_state_t));

    ps->user_mode.regs.rflags = PROC_DEFAULT_RFLAGS;
    ps->user_mode.regs.cs = get_user_cs() | PL3;
    ps->user_mode.regs.ss = get_user_ds() | PL3;
}

static int process_load_pml4(process_t* ps)
{
    uint64_t paddr, vaddr;

    paddr = request_page();
    if 
    (
        paddr == 0 || 
        paging_get_next_vaddr(SIZE_4KB, &vaddr) < SIZE_4KB ||
        paging_map_memory(paddr, vaddr, SIZE_4KB, ACCESS_RW, PL0) < SIZE_4KB
    )
        return -1;

    ps->pml4 = (page_table_t) vaddr;
    ps->pml4_paddr = paddr;
    memset(ps->pml4, 0, SIZE_4KB);

    return 0;
}

static int process_load_code(process_t* ps, const char* path)
{
    /**
     *  IDFK what the fuck i'm supposed to here, like
     *  how the fuck do i load an executable if i can't have a fs driver 
     *  in my fucking micro ass kernel like,
     *  idk man seems kinda sketch
     *  am i even allowed to have a vfs in this thing or do i have to
     *  make a fucking file server that i then have to launch from my fucking
     *  kernel which also needs a vfs in order to launch the file server that
     *  has to be launched from the kernel that needs a vfs to launch the thing majing
     *  idfk anymore just why do i make shit difficult for myself
     */

    return 0;
}

process_t* create_process(const char* path, uint64_t pid)
{
    process_t* ps = malloc(sizeof(process_t));
    if (ps == NULL)
        return NULL;

    init_process(ps, pid);

    if 
    (
        process_load_pml4(ps) ||
        process_load_code(ps, path)
    )
    {
        process_delete_and_free(ps);
    }
}