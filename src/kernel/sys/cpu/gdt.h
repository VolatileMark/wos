#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>

void gdt_init(void);
uint16_t gdt_get_kernel_cs(void);
uint16_t gdt_get_kernel_ds(void);
uint16_t gdt_get_user_cs(void);
uint16_t gdt_get_user_ds(void);
uint16_t gdt_get_tss_ss(void);
#endif