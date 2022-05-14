#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "process.h"

int scheduler_init(void);
uint64_t scheduler_get_next_pid(void);
int scheduler_queue_process(process_t* ps);
int scheduler_replace_process(process_t* old, process_t* new);
int scheduler_terminate_process(process_t* ps);
process_t* scheduler_get_current_process(void);
void scheduler_run(void);

#endif