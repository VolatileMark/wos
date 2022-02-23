#ifndef __MACROS_H__
#define __MACROS_H__

#include "constants.h"
#include <stdint.h>

#define UNUSED(x) (void)(x)
#define HALT() while (1) { __asm__ ("hlt"); }
#define SPIN(times) { uint64_t __spin; for (__spin = 0; __spin++ < times; __spin++); }
#define SIZE_nMB(n) (n * SIZE_1MB)

#endif