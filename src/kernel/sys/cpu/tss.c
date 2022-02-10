#include "tss.h"
#include "../../utils/constants.h"
#include <mem.h>

static __attribute__((aligned(SIZE_4KB))) tss_t tss;

void init_tss(void)
{
    memset(&tss, 0, sizeof(tss_t));
}

void tss_set_kernel_stack(uint64_t stack_vaddr)
{
    tss.rsp0 = stack_vaddr;
}

tss_t* get_tss(void)
{
    return &tss;
}