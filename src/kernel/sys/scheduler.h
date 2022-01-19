#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdint.h>
#include "process.h"
#include "../cpu/isr.h"

uint64_t get_next_pid(void);

int init_scheduler(void);
void run_scheduler(void);
process_t* get_current_scheduled_process(void);

int schedule_runnable_process(process_t* ps);
int replace_process(process_t* old, process_t* new);
void terminate_process(process_t* ps);
int has_any_child_process_terminated(process_t* parent);
int num_children_of_process(uint64_t pid);
void set_init_exec_file(uint64_t exec_paddr, uint64_t exec_size);

extern void snapshot_and_schedule(cpu_state_t* current);

#endif