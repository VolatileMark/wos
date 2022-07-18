#ifndef __GDT_H__
#define __GDT_H__

#include <stdint.h>

struct gdt_descriptor 
{
    uint16_t size;
    uint64_t addr;
} __attribute__((packed));
typedef struct gdt_descriptor gdt_descriptor_t;

void gdt_init(void);
uint16_t gdt_get_kernel_cs(void);
uint16_t gdt_get_kernel_ds(void);
uint16_t gdt_get_user_cs(void);
uint16_t gdt_get_user_ds(void);
uint16_t gdt_get_tss_ss(void);
gdt_descriptor_t* gdt_get_descriptor(void);

#endif
