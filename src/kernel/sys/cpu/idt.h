#ifndef __IDT_H__
#define __IDT_H__

#include <stdint.h>

#define IDT_NUM_ENTRIES 256

struct idt64_descriptor 
{
    uint16_t limit;         /* Size in bytes of the IDT - 1 */
    uint64_t base;          /* Where the IDT starts (base address) */
} __attribute__((packed));
typedef struct idt64_descriptor idt64_descriptor_t;

void idt_init(void);
void idt_set_interrupt_present(uint8_t interruptNumber, uint8_t value);
idt64_descriptor_t* idt_get_descriptor(void);

#endif