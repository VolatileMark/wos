#include "tty.h"
#include "multiboot2-utils.h"
#include "psf.h"
#include "log.h"
#include "../sys/drivers/video/framebuffer.h"
#include <stdarg.h>
#include <string.h>

static uint32_t cursor_x, cursor_y;
static uint32_t max_x, max_y;
static uint32_t offset_x, offset_y;
static uint32_t fg_color, bg_color;
static framebuffer_info_t* fb;
extern volatile psf2_file_t _binary_font_psf_start;

#define font_header _binary_font_psf_start.header
#define font_glyphs _binary_font_psf_start.glyphs

static void putc(char c)
{
    uint64_t glyph_offset;
    uint64_t x, y, sx, sy;
    uint32_t color;
    uint8_t glyph_byte;

    switch (c)
    {
    case '\n':
        cursor_x = 0;
        ++cursor_y;
        break;
    default:
        glyph_offset = ((uint8_t) c) * font_header.bytes_per_glyph;
        
        sx = (cursor_x + offset_x) * font_header.width;
        sy = (cursor_y + offset_y) * font_header.height;

        for (y = 0; y < font_header.height; y++)
        {
            glyph_byte = font_glyphs[glyph_offset + y];
            for (x = font_header.width; x > 0; x--)
            {
                if (glyph_byte & 1)
                    color = fg_color;
                else
                    color = bg_color;
                FB_PIXEL(fb, sx + (x - 1), sy + y) = color;
                glyph_byte >>= 1;
            }
        }
        ++cursor_x;
        break;
    }

    if (cursor_x >= max_x)
    {
        cursor_x = 0;
        ++cursor_y;
    }
    if (cursor_y >= max_y)
        cursor_y = 0;
}

static void puts(char* str)
{
    while (*str != '\0')
        putc(*str++);
}

void printf(const char* str, ...)
{
    va_list ap;
    char* p;
    char tmp_str[64];

    va_start(ap, str);

    for (p = ((char*) str); *p != '\0'; p++)
    {
        if (*p != '%')
        {
            putc(*p);
            continue;
        }

        tmp_str[0] = '\0';
        switch (*(++p))
        {
        case 'c':
            putc((uint8_t) va_arg(ap, uint32_t));
            break;
        case 'p':
            puts(utoan(va_arg(ap, uint64_t), tmp_str, 16, 16));
            break;
        case 'x':
            puts(itoa(va_arg(ap, long), tmp_str, 16));
            break;
        case 'd':
        case 'i':
            puts(itoa(va_arg(ap, long), tmp_str, 10));
            break;
        case 'f':
            break;
        case 's':
            puts(va_arg(ap, char*));
            break;
        case 'u':
            puts(utoa(va_arg(ap, uint64_t), tmp_str, 10));
            break;
        case '%':
            putc('%');
            break;
        }
    }

    va_end(ap);
}

void clear_screen(void)
{
    uint32_t* fb_ptr;
    uint64_t i;

    fb_ptr = (uint32_t*) fb->addr;
    for (i = 0; i < fb->size; i++)
        fb_ptr[i] = bg_color;
}

int tty_init(void)
{
    fb = framebuffer_get();

    cursor_x = 0;
    cursor_y = 0;

    if 
    (
        font_header.magic != PSF2_FONT_MAGIC ||
        font_header.version != 0 ||
        font_header.flags != 0 ||
        font_header.width != 8
    )
        return -1;
    
    max_x = fb->width / font_header.width;
    max_y = fb->height / font_header.height;
    if (!(max_x && max_y))
        return -1;

    offset_x = 0;
    offset_y = 0;

    fg_color = framebuffer_color(255, 255, 255);
    bg_color = framebuffer_color(0, 0, 0);

    info("Framebuffer located at %p, mapped at %p, has size %u bytes", multiboot_get_tag_framebuffer()->common.framebuffer_addr, fb->addr, fb->size);
    info("TTY initialized. Resolution is %ux%u pixels or %ux%u characters", fb->width, fb->height, max_x, max_y);

    return 0;
}

void tty_resize_viewport(uint32_t width, uint32_t height)
{
    max_x = width / font_header.width;
    max_y = height / font_header.height;
}

void tty_offset_viewport(uint32_t x, uint32_t y)
{
    offset_x = x / font_header.width;
    offset_y = y / font_header.height;
}

void tty_set_cursor_pos(uint32_t cx, uint32_t cy)
{
    cursor_x = cx;
    cursor_y = cy;
}

int tty_is_initialized(void)
{
    return
    (
        font_header.magic == PSF2_FONT_MAGIC &&
        font_header.version == 0 &&
        font_header.flags == 0 &&
        font_header.width == 8 &&
        max_x && max_y
    );
}