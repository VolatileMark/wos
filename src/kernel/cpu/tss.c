#include "tss.h"
#include <mem.h>

__attribute__((aligned(0x1000))) tss_t tss;

uint64_t tss_init(void)
{
    memset(&tss, 0, sizeof(tss_t));
    return (uint64_t) &tss;
}

void tss_set_kernel_stack(uint64_t stack_vaddr)
{
    tss.rsp0 = stack_vaddr;
}