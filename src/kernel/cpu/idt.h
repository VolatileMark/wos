#ifndef __IDT_H__
#define __IDT_H__

#include <stdint.h>

#define IDT_NUM_ENTRIES 256

void set_idt_interrupt_present(uint8_t interruptNumber, uint8_t value);
void idt_init(void);

#endif