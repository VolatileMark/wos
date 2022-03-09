#ifndef __PIC_H__
#define __PIC_H__

#include <stdint.h>

#define IRQ_COUNT 16

/**
 * Initialize the PIC
 */
void pic_init(void);

/**
 * Acknowledge interrupt
 */
void pic_acknowledge(uint8_t int_num);

void pic_mask_irq(uint8_t index);
void pic_unmask_irq(uint8_t index);
void pic_toggle_irq(uint8_t index);

#endif