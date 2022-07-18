#include "isr.h"
#include "idt.h"
#include "handlers.h"
#include "../chips/pic.h"
#include "../../utils/log.h"
#include <stddef.h>
#include <mem.h>

#define trace_isr(msg, ...) trace("IRQS", msg, ##__VA_ARGS__)

static isr_handler_t isr_handlers[IDT_NUM_ENTRIES];

void isr_init(void)
{
    memset(isr_handlers, 0, IDT_NUM_ENTRIES * sizeof(isr_handler_t));
    isr_register_handler(EXCEPTION_PF, &handler_pf);
}

void isr_handler(interrupt_frame_t* frame)
{
    uint64_t interrupt_number;
    interrupt_number = frame->interrupt_info.interrupt_number;
    if (isr_handlers[interrupt_number] != NULL)
    {
        isr_handlers[interrupt_number](frame);
        if (interrupt_number >= IRQ(0))
            pic_acknowledge(interrupt_number);
    }
    else if (interrupt_number < IRQ(0))
        panic(frame, "Unhandled exception %02X", interrupt_number);
    else
    {
        trace_isr("Unhandled interrupt %02X", interrupt_number);
        pic_acknowledge(interrupt_number);
    }
}

void isr_register_handler(uint8_t interrupt_number, isr_handler_t handler)
{
    /* Allow overwriting handlers (idk might be useful) */
    isr_handlers[interrupt_number] = handler;
    idt_set_interrupt_present(interrupt_number, 1);
    if (interrupt_number >= IRQ(0))
        pic_unmask_irq(interrupt_number);
}
