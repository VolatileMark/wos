#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdint.h>
#include "process.h"
#include "../sys/cpu/isr.h"

uint64_t scheduler_get_next_pid(void);

int scheduler_init(void);
void scheduler_run(void);
void scheduler_delete_zombie_processes(void);
process_t* scheduler_get_current_scheduled_process(void);

int scheduler_schedule_runnable_process(process_t* ps);
int scheduler_replace_process(process_t* old, process_t* new);
void scheduler_terminate_process(process_t* ps);
int scheduler_has_any_child_process_terminated(process_t* parent);
int scheduler_get_num_children_of_process(uint64_t pid);
void set_init_exec_file(uint64_t exec_paddr, uint64_t exec_size);

extern void snapshot_and_schedule(cpu_state_t* current);

#endif