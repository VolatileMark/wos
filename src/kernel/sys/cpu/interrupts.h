#ifndef __INTERRUPTS_H__
#define __INTERRUPTS_H__

#define interrupts_enable() __asm__("sti")
#define interrupts_disable() __asm__("cli")

#endif