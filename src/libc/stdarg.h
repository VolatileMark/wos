#ifndef __STDARG_H__
#define __STDARG_H__

#define va_list __builtin_va_list
#define va_start(v, l) __builtin_va_start(v, l)
#define va_arg(v, t) __builtin_va_arg(v, t)
#define va_end(v) __builtin_va_end(v)

#endif