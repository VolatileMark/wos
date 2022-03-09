#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdint.h>

int screen_init(void);

void screen_resize_viewport(uint32_t width, uint32_t height);
void screen_offset_viewport(uint32_t x, uint32_t y);
void screen_set_cursor_pos(uint32_t cx, uint32_t cy);

void printf(const char* str, ...);

int is_screen_initialized(void);

#endif