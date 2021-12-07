#include "pic.h"
#include "../io/ports.h"
#include "../../cpu/isr.h"

#define IRQ_COUNT 16

#define PIC1 0x20
#define PIC2 0xA0

#define PIC1_COMM PIC1
#define PIC1_DATA PIC1 + 1

#define PIC2_COMM PIC2
#define PIC2_DATA PIC2 + 1

/**
 *  Bit 0: If 1, the PIC expects ICW4.
 *  Bit 1: If 1, there's only 1 PIC in the system (only 8 IRQs are available).
 *  Bit 2: Ignored by x86 and set to 0 by default.
 *  Bit 3: If 1, the PIC should operate in Level Triggered Mode, otherwise it should operate in Edge Triggered Mode.
 *  Bit 4: 1 if the PIC is to be initialized.
 *  Bit 5-7: Must be 0
 */
#define PIC_ICW1 0b00010001 /* Same for both PICs */

/**
 *  Set PIC1 to start at interrupt 32 (IRQ 0).
 *  Each PIC handles 8 interrupts, starting from
 *  the one specified by ICW2.
 *  In this case PIC1 will cover:
 *  IRQ0, IRQ1, IRQ2, IRQ3, IRQ4, IRQ5, IRQ6, IRQ7
 */
#define PIC1_ICW2 IRQ(0) /* = 32 = 0x20 */

/**
 *  Set PIC2 to start at interrupt 40 (IRQ 8).
 *  Each PIC handles 8 interrupts, starting from
 *  the one specified by ICW2.
 *  In this case PIC2 will cover:
 *  IRQ8, IRQ9, IRQ10, IRQ11, IRQ12, IRQ13, IRQ14, IRQ15
 */
#define PIC2_ICW2 IRQ(8) /* = 40 = 0x28 */

/**
 *  Tells PIC1 which IRQ it should use to
 *  communicate with PIC2.
 *  Bit 0: IRQ 0
 *  Bit 1: IRQ 1
 *  Bit 2: IRQ 2
 *  Bit 3: IRQ 3
 *  Bit 4: IRQ 4
 *  Bit 5: IRQ 5
 *  Bit 6: IRQ 6
 *  Bit 7: IRQ 7
 *  In this case it should use IRQ2.
 */
#define PIC1_ICW3 0b00000100

/**
 *  Tells PIC1 which IRQ it should use to
 *  communicate with PIC2.
 *  Bit 0-2: IRQ line in binary notation.
 *  Bit 3-7: Reserved and must be 0
 *  The code that should be put in bits 0-2
 *  should be:
 *      000: IRQ 0
 *      001: IRQ 1
 *      010: IRQ 2
 *      011: IRQ 3
 *      100: IRQ 4
 *      101: IRQ 5
 *      110: IRQ 6
 *      111: IRQ 7
 *  In this case it should use IRQ2.
 */
#define PIC2_ICW3 0b00000010

/**
 *  Sets the mode in which the PIC should operate.
 *  Bit 0: If 1, the PIC operates in x86 mode.
 *  Bit 1: If 1, the PIC automatically sends the acknowledge pulse.
 *  Bit 2: If 1, this PIC is the buffer master, otherwise the PIC is the buffer slave. !!! Only use if bit 3 is 1. !!!
 *  Bit 3: If 1, the PIC operates in buffer mode.
 *  Bit 4: If 1, the PIC operates in Special Fully !!! Nested Mode. Not supported by x86. !!!
 *  Bit 5-7: Reserved, must be 0.
 */ 
#define PIC_ICW4 0x00000001 /* Same for both PICs */

#define PIC_EOI 0x20



void init_pic(void)
{
    /* Send input control word no. 1 */
    ports_wait();
    ports_write_byte(PIC1_COMM, PIC_ICW1);
    ports_wait();
    ports_write_byte(PIC2_COMM, PIC_ICW1);
    
    /* Send input control word no. 2 */
    ports_wait();
    ports_write_byte(PIC1_DATA, PIC1_ICW2);
    ports_wait();
    ports_write_byte(PIC2_DATA, PIC2_ICW2);

    /* Send input control word no. 3 */
    ports_wait();
    ports_write_byte(PIC1_DATA, PIC1_ICW3);
    ports_wait();
    ports_write_byte(PIC2_DATA, PIC2_ICW3);

    /* Send input control word no. 4 */
    ports_wait();
    ports_write_byte(PIC1_DATA, PIC_ICW4);
    ports_wait();
    ports_write_byte(PIC2_DATA, PIC_ICW4);

    /* Null out the data registers */
    ports_wait();
    ports_write_byte(PIC1_DATA, 0);
    ports_wait();
    ports_write_byte(PIC2_DATA, 0);

    /* Disable ALL IRQs (Do not set bit 2 as it's the one that enables PIC2) */
    ports_wait();
    ports_write_byte(PIC1_DATA, 0b11111011);
    ports_wait();
    ports_write_byte(PIC2_DATA, 0b11111111);
}

void pic_mask_irq(uint8_t index)
{
    /* Check if the index is absolute or relative to IRQ0 */
    if (index >= IRQ_COUNT)
        index -= IRQ(0);
    /* Check if the index is valid */
    if (index < 0)
        return;
    
    uint8_t pic_mask;
    uint8_t bit_mask;
    uint8_t pic_port = PIC1_DATA;
    /* Check if the IRQ is handled by PIC2 */
    if (index >= IRQ_COUNT / 2)
    {
        pic_port = PIC2_DATA;
        index -= IRQ_COUNT / 2;
    }

    bit_mask = 1 << index;
    pic_mask = ports_read_byte(pic_port);
    pic_mask |= bit_mask;
    ports_wait();
    ports_write_byte(pic_port, pic_mask);
}

void pic_unmask_irq(uint8_t index)
{
    /* Check if the index is absolute or relative to IRQ0 */
    if (index >= IRQ_COUNT)
        index -= IRQ(0);
    /* Check if the index is valid */
    if (index < 0)
        return;
    
    uint8_t pic_mask;
    uint8_t bit_mask;
    uint8_t pic_port = PIC1_DATA;
    /* Check if the IRQ is handled by PIC2 */
    if (index >= IRQ_COUNT / 2)
    {
        pic_port = PIC2_DATA;
        index -= IRQ_COUNT / 2;
    }
    bit_mask = 1 << index;
    pic_mask = ports_read_byte(pic_port);
    pic_mask &= ~bit_mask;
    ports_wait();
    ports_write_byte(pic_port, pic_mask);
}

void pic_toggle_irq(uint8_t index)
{
    /* Check if the index is absolute or relative to IRQ0 */
    if (index >= IRQ_COUNT)
        index -= IRQ(0);
    /* Check if the index is valid */
    if (index < 0)
        return;
    
    uint8_t pic_mask;
    uint8_t bit_mask;
    uint8_t pic_port = PIC1_DATA;
    /* Check if the IRQ is handled by PIC2 */
    if (index >= IRQ_COUNT / 2)
    {
        pic_port = PIC2_DATA;
        index -= IRQ_COUNT / 2;
    }

    bit_mask = 1 << index;
    pic_mask = ports_read_byte(pic_port);
    pic_mask ^= bit_mask;
    ports_wait();
    ports_write_byte(pic_port, pic_mask);
}

void pic_acknowledge(uint8_t interrupt_number)
{
    if (interrupt_number >= IRQ(IRQ_COUNT / 2))
        ports_write_byte(PIC2_COMM, PIC_EOI);
    ports_write_byte(PIC1_COMM, PIC_EOI);
}