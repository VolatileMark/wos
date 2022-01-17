#include "scheduler.h"
#include <stdint.h>
#include <stdarg.h>
#include <syscall.h>

void syscall_handler(uint64_t num, ...)
{
    va_list ap;
    va_start(ap, num);

    switch (num)
    {
    case SYS_exit:
        break;
    }

    va_end(ap);
}