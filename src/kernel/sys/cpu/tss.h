#ifndef __TSS_H__
#define __TSS_H__ 1

#include <stdint.h>

struct tss
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb;
} __attribute__((packed));
typedef struct tss tss_t;

void tss_init(void);
void tss_set_kernel_stack(uint64_t stack_vaddr);
tss_t* tss_get(void);
extern void tss_load(uint16_t tss_seg_sel);

#endif
