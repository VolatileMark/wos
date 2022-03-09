#include "scheduler.h"
#include "../sys/cpu/gdt.h"
#include "../sys/cpu/isr.h"
#include "../sys/cpu/tss.h"
#include "../sys/cpu/interrupts.h"
#include "../utils/constants.h"
#include "../sys/chips/pic.h"
#include "../sys/chips/pit.h"
#include "../utils/macros.h"
#include "../utils/alloc.h"
#include "../utils/log.h"
#include "../utils/elf.h"
#include <stddef.h>
#include <math.h>
#include <mem.h>

#define trace_scheduler(msg, ...) trace("SCHD", msg, ##__VA_ARGS__)

#define SCHEDULER_PIT_INTERVAL 2 /* ms */
#define SCHEDULER_TIME_SLICE (5 * SCHEDULER_PIT_INTERVAL)

typedef struct process_list_entry
{
    struct process_list_entry* next;
    process_t* ps;
} process_list_entry_t;

typedef struct process_list
{
    process_list_entry_t* head;
    process_list_entry_t* tail;
} process_list_t;

static uint64_t ms;
static process_list_t runnable_pss, zombie_pss;

extern void run_process_in_user_mode(cpu_state_t* cpu);
extern void run_process_in_kernel_mode(cpu_state_t* cpu);
extern void switch_pml4_and_stack(uint64_t pml4_paddr);

static uint64_t max_id_in_process_list(process_list_t* pss)
{
    uint64_t max;
    process_list_entry_t* ptr;
    
    max = 0;
    ptr = pss->head;
    while (ptr != NULL)
    {
        if (ptr->ps != NULL && ptr->ps->pid >= max)
            max = ptr->ps->pid;
        ptr = ptr->next;
    }
    return max;
}

uint64_t scheduler_get_next_pid(void)
{
    uint64_t pid_runnable, pid_zombie;
    pid_runnable = max_id_in_process_list(&runnable_pss);
    pid_zombie = max_id_in_process_list(&zombie_pss);
    return maxu(pid_runnable, pid_zombie) + 1;
}

static void copy_common_registers(cpu_state_t* cpu, const registers_state_t* regs, const stack_state_t* stack)
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

static void update_user_registers(process_t* ps, const registers_state_t* regs, const stack_state_t* stack)
{
    copy_common_registers(&ps->user_mode, regs, stack);
    ps->user_mode.stack.rsp = stack->rsp;
    ps->user_mode.stack.ss = stack->ss;
    ps->current = ps->user_mode;
}

static void update_kernel_registers(process_t* ps, const registers_state_t* regs, const stack_state_t* stack)
{
    copy_common_registers(&ps->current, regs, stack);
    ps->current.stack.rsp = stack->rsp + (3 * sizeof(uint64_t));
    ps->current.stack.ss = gdt_get_kernel_ds();
}

static void scheduler_schedule_on_interrupt(const registers_state_t* regs, const stack_state_t* stack, uint8_t interrupt_number)
{
    process_t* ps;
    interrupts_disable();
    ps = scheduler_get_current_scheduled_process();
    if (stack->cs == (gdt_get_user_cs() | PL3))
        update_user_registers(ps, regs, stack);
    else
        update_kernel_registers(ps, regs, stack);
    pic_acknowledge(interrupt_number);
    scheduler_run();
}

static void scheduler_pit_handler(const interrupt_frame_t* int_frame)
{
    ms += SCHEDULER_PIT_INTERVAL;
    if (ms >= SCHEDULER_TIME_SLICE)
    {
        ms = 0;
        scheduler_schedule_on_interrupt(&int_frame->registers_state, &int_frame->stack_state, (uint8_t) int_frame->interrupt_info.interrupt_number);
    }
    else
        pic_acknowledge((uint8_t) int_frame->interrupt_info.interrupt_number);
}

static void scheduler_yield_handler(const interrupt_frame_t* int_frame)
{
    ms = 0;
    scheduler_schedule_on_interrupt(&int_frame->registers_state, &int_frame->stack_state, (uint8_t) int_frame->interrupt_info.interrupt_number);
}

int scheduler_init(void)
{
    ms = 0;
    memset(&runnable_pss, 0, sizeof(process_list_t));
    memset(&zombie_pss, 0, sizeof(process_list_t));
    set_pit_interval(SCHEDULER_PIT_INTERVAL);
    isr_register_handler(128, &scheduler_yield_handler);
    return register_pit_callback(&scheduler_pit_handler);
}

process_t* scheduler_get_current_scheduled_process(void)
{
    return (runnable_pss.head == NULL) ? NULL : runnable_pss.head->ps;
}

static int scheduler_schedule_process(process_list_t* pss, process_t* ps)
{
    process_list_entry_t* entry;
    
    entry = malloc(sizeof(process_list_entry_t));
    if (entry == NULL)
    {
        trace_scheduler("Could not allocate space for process entry");
        return -1;
    }
    
    entry->ps = ps;
    entry->next = NULL;

    if (pss->head == NULL)
        pss->head = entry;
    else
        pss->tail->next = entry;
    pss->tail = entry;

    return 0;
}

int scheduler_schedule_runnable_process(process_t* ps)
{
    return scheduler_schedule_process(&runnable_pss, ps);
}

static int scheduler_remove_process(process_list_t* pss, uint64_t pid, uint8_t should_delete)
{
    process_list_entry_t* current;
    process_list_entry_t* prev;
    
    current = pss->head;
    prev = NULL;
    while (current != NULL)
    {
        if (current->ps != NULL && current->ps->pid == pid)
        {
            if (prev == NULL)
                pss->head = current->next;
            else
                prev->next = current->next;
            
            if (pss->tail == current)
            {
                pss->tail = prev;
                prev->next = NULL;
            }

            if (should_delete)
                process_delete_and_free(current->ps);
            
            free(current);
            
            return 0;
        }
    
        prev = current;
        current = current->next;
    }

    trace_scheduler("Process %u not found. Could not remove process", pid);

    return -1;
}

void scheduler_terminate_process(process_t* ps)
{
    process_delete_resources(ps);
    scheduler_remove_process(&runnable_pss, ps->pid, 0);
    scheduler_schedule_process(&zombie_pss, ps);
}

int scheduler_has_any_child_process_terminated(process_t* parent)
{
    process_list_entry_t* entry;
    
    entry = zombie_pss.head;
    while (entry != NULL)
    {
        if (entry->ps->parent_pid == parent->pid)
            return 1;
        entry = entry->next;
    }

    return 0;
}

static int count_num_children_of_process_in_list(process_list_t* pss, uint64_t parent_pid)
{
    int num;
    process_list_entry_t* entry;
    
    num = 0;
    entry = pss->head;
    while (entry != NULL)
    {
        if (entry->ps->parent_pid == parent_pid)
            ++num;
        entry = entry->next;
    }
    return num;
}

int scheduler_get_num_children_of_process(uint64_t parent_pid)
{
    int num_running, num_zombies;
    num_running = count_num_children_of_process_in_list(&runnable_pss, parent_pid);
    num_zombies = count_num_children_of_process_in_list(&zombie_pss, parent_pid);
    return (num_zombies + num_running);
}

int scheduler_replace_process(process_t* old, process_t* new)
{
    if (scheduler_remove_process(&runnable_pss, old->pid, 1))
    {
        trace_scheduler("Failed to replace process %u", old->pid);
        return -1;
    }
    return scheduler_schedule_runnable_process(new);
}

static process_t* get_next_runnable_process(void)
{
    process_list_entry_t* proc;

    if (runnable_pss.head == NULL || runnable_pss.head->ps == NULL)
        return NULL;
    
    proc = runnable_pss.head;
    if (proc->next != NULL)
    {
        runnable_pss.head = proc->next;
        proc->next = NULL;
        runnable_pss.tail->next = proc;
        runnable_pss.tail = proc;
    }

    return runnable_pss.head->ps;
}

void scheduler_delete_zombie_processes(void)
{
    /* Delete all zombie processes */
    process_list_entry_t* tmp;
    process_list_entry_t* ptr;
    
    ptr = zombie_pss.head;
    while (ptr != NULL)
    {
        if (ptr->ps != NULL)
        {
            free(ptr->ps);
            ptr->ps = NULL;
        }
        tmp = ptr->next;
        free(ptr);
        ptr = tmp;
        zombie_pss.head = ptr;
    }

    zombie_pss.tail = zombie_pss.head;
}

void scheduler_run(void)
{
    process_t* ps;
    
    ps = get_next_runnable_process();
    if (ps == NULL)
    {
        trace_scheduler("No process to execute. Halting...");
        HALT();
    }
    
    tss_set_kernel_stack(ps->kernel_stack_start_vaddr + PROC_INITIAL_STACK_SIZE - sizeof(uint64_t));
    kernel_inject_pml4(ps->pml4);
    switch_pml4_and_stack(ps->pml4_paddr);

    if (ps->current.stack.cs == gdt_get_kernel_cs())
        run_process_in_kernel_mode(&ps->current);
    else
        run_process_in_user_mode(&ps->current);
}