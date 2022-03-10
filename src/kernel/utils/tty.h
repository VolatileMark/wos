#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdint.h>

int tty_init(void);

void tty_resize_viewport(uint32_t width, uint32_t height);
void tty_offset_viewport(uint32_t x, uint32_t y);
void tty_set_cursor_pos(uint32_t cx, uint32_t cy);

void printf(const char* str, ...);

int tty_is_initialized(void);

#endif