#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

#include <stdint.h>

#define FB_PIXEL(fb, x, y) *((uint32_t*) (fb->addr + ((y) * fb->pitch) + ((x) * fb->bytes_per_pixel)))

typedef struct
{
    uint64_t addr;
    uint64_t size;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint64_t bytes_per_pixel;
    uint32_t red_mask;
    uint32_t red_offset;
    uint32_t green_mask;
    uint32_t green_offset;
    uint32_t blue_mask;
    uint32_t blue_offset;
} framebuffer_info_t;

int framebuffer_init(void);
int put_pixel(uint32_t x, uint32_t y, uint32_t color);
framebuffer_info_t* framebuffer_get(void);
uint32_t framebuffer_width(void);
uint32_t framebuffer_height(void);
uint64_t framebuffer_vaddr(void);
uint64_t framebuffer_size(void);
uint32_t framebuffer_color(uint8_t r, uint8_t g, uint8_t b);

#endif
