#include "scheduler.h"
#include "../utils/alloc.h"
#include "../utils/log.h"
#include "../sys/chips/pic.h"
#include "../sys/chips/pit.h"
#include "../sys/cpu/tss.h"
#include "../sys/cpu/interrupts.h"
#include <stddef.h>
#include <math.h>
#include <mem.h>

#define SCHEDULER_PIT_INTERVAL 2
#define SCHEDULER_TIME_PER_PROC (5 * SCHEDULER_PIT_INTERVAL)

#define trace_scheduler(msg, ...) trace("SCHD", msg, ##__VA_ARGS__)

typedef struct process_list_entry
{
    struct process_list_entry* next;
    process_t* ps;
} process_list_entry_t;

typedef struct
{
    process_list_entry_t* head;
    process_list_entry_t* tail;
} process_list_t;

static process_list_t zombie;
static process_list_t running;
static uint64_t ms;

extern process_t* scheduler_switch_pml4_and_stack(process_t* ps, uint64_t rsp);
extern void scheduler_run_process(cpu_state_t* cpu);

static uint64_t scheduler_get_max_pid_in_process_list(process_list_t* pss)
{
    uint64_t pid;
    process_list_entry_t* entry;
    for (entry = pss->head, pid = 0; entry != NULL; entry = entry->next)
    {
        if (pid < entry->ps->pid)
            pid = entry->ps->pid;
    }
    return pid;
}

uint64_t scheduler_get_next_pid(void)
{
    uint64_t pid_running, pid_zombie;
    pid_running = scheduler_get_max_pid_in_process_list(&running);
    pid_zombie = scheduler_get_max_pid_in_process_list(&zombie);
    return (maxu(pid_running, pid_zombie) + 1);
}

process_t* scheduler_get_current_process(void)
{
    return (running.head == NULL) ? NULL : running.head->ps;
}

static void scheduler_update_registers(cpu_state_t* cpu, const registers_state_t* regs, const stack_state_t* stack)
{
    cpu->regs.rax = regs->rax;
    cpu->regs.rbx = regs->rbx;
    cpu->regs.rcx = regs->rcx;
    cpu->regs.rdx = regs->rdx;
    cpu->regs.r8 = regs->r8;
    cpu->regs.r9 = regs->r9;
    cpu->regs.r10 = regs->r10;
    cpu->regs.r11 = regs->r11;
    cpu->regs.r12 = regs->r12;
    cpu->regs.r13 = regs->r13;
    cpu->regs.r14 = regs->r14;
    cpu->regs.r15 = regs->r15;
    cpu->regs.rbp = regs->rbp;
    cpu->regs.rsi = regs->rsi;
    cpu->regs.rdi = regs->rdi;
    cpu->stack.rip = stack->rip;
    cpu->stack.rflags = stack->rflags;
    cpu->stack.cs = stack->cs;
}

static void scheduler_handle_interrupt(const interrupt_frame_t* int_frame)
{
    process_t* ps;
    uint64_t ps_fpu, frame_fpu;
    interrupts_disable();
    ps = scheduler_get_current_process();
    ps_fpu = alignu((uint64_t) ps->cpu.fpu, 16);
    frame_fpu = alignu((uint64_t) int_frame->fpu_state, 16);
    memcpy((void*) ps_fpu, (void*) frame_fpu, 512);
    scheduler_update_registers(&ps->cpu, &int_frame->registers_state, &int_frame->stack_state);
    pic_acknowledge(int_frame->interrupt_info.interrupt_number);
    scheduler_run();
}

static void scheduler_pit_handler(const interrupt_frame_t* int_frame)
{
    ms += SCHEDULER_PIT_INTERVAL;
    if (ms >= SCHEDULER_TIME_PER_PROC)
    {
        ms = 0;
        scheduler_handle_interrupt(int_frame);
    }
    else
        pic_acknowledge((uint8_t) int_frame->interrupt_info.interrupt_number);
}

void scheduler_init_pss(void)
{
    memset(&running, 0, sizeof(process_list_t));
    memset(&zombie, 0, sizeof(process_list_t));
}

int scheduler_init(void)
{
    ms = 0;
    pit_set_interval(SCHEDULER_PIT_INTERVAL);
    return pit_register_callback(&scheduler_pit_handler);
}

static int scheduler_queue_process_in_list(process_list_t* pss, process_t *ps)
{
    process_list_entry_t* entry;

    entry = malloc(sizeof(process_list_entry_t));
    if (entry == NULL)
    {
        trace_scheduler("Could not allocate space for process list entry");
        return -1;
    }
    
    entry->ps = ps;
    entry->next = NULL;

    if (pss->tail == NULL)
        pss->head = entry;
    else
        pss->tail->next = entry;
    pss->tail = entry;

    return 0;
}

static int scheduler_remove_process_from_list(process_list_t* pss, process_t* ps)
{
    process_list_entry_t* current;
    process_list_entry_t* prev;

    current = pss->head;
    prev = NULL;

    while (current != NULL)
    {
        if (current->ps != NULL && current->ps->pid == ps->pid)
        {
            if (prev == NULL)
                pss->head = current->next;
            else
                prev->next = current->next;

            if (current == pss->tail)
            {
                pss->tail = prev;
                if (prev != NULL)
                    prev->next = NULL;
            }

            free(current);
            return 0;
        }
        prev = current;
        current = current->next;
    }

    return -1;
}

int scheduler_replace_process(process_t* old, process_t* new)
{
    if 
    (
        scheduler_remove_process_from_list(&running, old) ||
        scheduler_queue_process_in_list(&running, new)
    )
    {
        trace_scheduler("Failed to replace process");
        return -1;
    }
    return 0;
}

int scheduler_queue_process(process_t* ps)
{
    return scheduler_queue_process_in_list(&running, ps);
}

int scheduler_terminate_process(process_t* ps)
{
    if 
    (
        ps == NULL ||
        scheduler_remove_process_from_list(&running, ps)
    )
        return -1;
    process_delete_resources(ps);
    return scheduler_queue_process_in_list(&zombie, ps);
}

static process_t* scheduler_fetch_next_running_process(void)
{
    process_list_entry_t* entry;

    if (running.head == NULL || running.head->ps == NULL)
        return NULL;
    
    entry = running.head;
    if (entry->next != NULL)
    {
        running.head = entry->next;
        entry->next = NULL;
        running.tail->next = entry;
        running.tail = entry;
    }

    return running.head->ps;
}

void scheduler_run(void)
{
    process_t* ps;

START_SCHEDULING:
    ps = scheduler_fetch_next_running_process();
    if (ps == NULL)
    {
        trace_scheduler("No process to execute. Halting...");
        HALT();
    }

    if (paging_inject_kernel_pml4(ps->pml4))
    {
        trace_scheduler("Failed to inject kernel PML4 (pid %u). Continuing...", ps->pid);
        goto START_SCHEDULING;
    }
    tss_set_kernel_stack(ps->kernel_stack.floor - sizeof(uint64_t));
    ps = scheduler_switch_pml4_and_stack(ps, tss_get()->rsp0);
    scheduler_run_process(&ps->cpu);
}