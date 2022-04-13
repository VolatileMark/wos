#ifndef __SCREEN_H__
#define __SCREEN_H__

#include <stdint.h>

int tty_init(void);

void tty_resize_viewport(uint32_t width, uint32_t height);
void tty_offset_viewport(uint32_t x, uint32_t y);
void tty_set_cursor_pos(uint32_t cx, uint32_t cy);

int tty_is_initialized(void);

void tty_printf(const char* str, ...);
void tty_clear_screen(void);

void tty_set_background_color(uint8_t r, uint8_t g, uint8_t b);
void tty_set_foreground_color(uint8_t r, uint8_t g, uint8_t b);


#endif