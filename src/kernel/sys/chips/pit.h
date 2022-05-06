#ifndef __PIT_H__
#define __PIT_H__

#include "../cpu/isr.h"
#include <stdint.h>

int pit_register_callback(isr_handler_t handler);
void pit_set_interval(uint64_t interval);
void pit_init(void);

#endif