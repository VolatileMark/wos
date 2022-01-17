#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>

void init_gdt(void);
uint16_t get_kernel_cs(void);
uint16_t get_kernel_ds(void);
uint16_t get_user_cs(void);
uint16_t get_user_ds(void);
uint16_t get_tss_ss(void);
#endif