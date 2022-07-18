#ifndef __MACROS_H__
#define __MACROS_H__

#include "../headers/constants.h"
#include <stdint.h>

#define UNUSED(x) (void)(x)
#define HALT() { while (1) { __asm__ ("hlt"); } }

#define SIZE_nKB(n) (n * SIZE_1KB)
#define SIZE_nMB(n) (n * SIZE_1MB)

#endif
