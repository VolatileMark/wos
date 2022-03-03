#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdint.h>

int init_screen(void);

void resize_screen_viewport(uint32_t width, uint32_t height);
void offset_screen_viewport(uint32_t x, uint32_t y);
void set_cursor_pos(uint32_t cx, uint32_t cy);

void printf(const char* str, ...);

int is_screen_initialized(void);

#endif