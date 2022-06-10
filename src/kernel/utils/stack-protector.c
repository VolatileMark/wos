#include "panic.h"
#include "macros.h"
#include <stdint.h>
#include <stddef.h>

#define STACK_CHK_GUARD 0x595e9fbd94fda766

uint64_t __stack_chk_guard = STACK_CHK_GUARD;

__attribute__((noreturn))
void __stack_chk_fail(void)
{
    panic(NULL, "Stack smashing detected");
}