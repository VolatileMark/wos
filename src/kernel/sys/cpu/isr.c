#include "isr.h"
#include "idt.h"
#include "../chips/pic.h"
#include <stddef.h>
#include <mem.h>

static isr_handler_t isr_handlers[IDT_NUM_ENTRIES];

void init_isr(void)
{
    memset(isr_handlers, 0, IDT_NUM_ENTRIES * sizeof(isr_handler_t));
}

void isr_handler(interrupt_frame_t* frame)
{
    uint64_t interrupt_number;
    interrupt_number = frame->interrupt_info.interrupt_number;
    if (isr_handlers[interrupt_number] != NULL)
    {
        isr_handlers[interrupt_number](frame);
        pic_acknowledge(interrupt_number);
    }
    else
        while (1);
}

uint8_t register_isr_handler(uint8_t interrupt_number, isr_handler_t handler)
{
    if (interrupt_number >= IDT_NUM_ENTRIES || isr_handlers[interrupt_number] != NULL)
        return 1;
    isr_handlers[interrupt_number] = handler;
    set_idt_interrupt_present(interrupt_number, 1);
    if (interrupt_number >= IRQ(0))
        pic_unmask_irq(interrupt_number);
    return 0;
}