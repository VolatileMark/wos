#ifndef __MACROS_H__
#define __MACROS_H__

#define UNUSED(x) (void)(x)
#define HALT() while (1) { __asm__ ("hlt"); }

#endif