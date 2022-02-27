#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

#include <stdint.h>

int init_framebuffer_driver(void);
int put_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);
uint32_t get_framebuffer_width(void);
uint32_t get_framebuffer_height(void);
uint64_t get_framebuffer_vaddr(void);
int is_framebuffer_driver_initialized(void);

#endif