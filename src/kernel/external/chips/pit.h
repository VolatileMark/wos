#ifndef __PIT_H__
#define __PIT_H__

#include "../../cpu/isr.h"
#include <stdint.h>

int register_pit_callback(isr_handler_t handler);
void set_pit_interval(uint64_t interval);
void init_pit(void);

#endif