#include "log.h"
#include "../psf.h"
#include "../../sys/drivers/video/framebuffer.h"
#include <stdarg.h>
#include <string.h>

static uint32_t cursor_x, cursor_y;
static uint32_t max_x, max_y;
extern volatile psf2_file_t _binary_font_psf_start;

#define font_header _binary_font_psf_start.header
#define font_glyphs _binary_font_psf_start.glyphs

static void putc(char c)
{
    uint64_t glyph_offset;
    uint8_t glyph_byte;
    uint64_t x, y, sx, sy;

    switch (c)
    {
    case '\n':
        cursor_x = 0;
        ++cursor_y;
        break;
    default:
        glyph_offset = c * font_header.bytes_per_glyph;
        
        sx = cursor_x * font_header.width;
        sy = cursor_y * font_header.height;

        for (y = 0; y < font_header.height; y++)
        {
            glyph_byte = font_glyphs[glyph_offset + y];
            for (x = font_header.width; x > 0; x--)
            {
                if (glyph_byte & 1)
                    put_pixel(sx + (x - 1), sy + y, 255, 255, 255);
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
        case 'x':
            puts(itoa((long) va_arg(ap, uint64_t), tmp_str, 16));
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
            puts(utoa(va_arg(ap, long), tmp_str, 10));
            break;
        case '%':
            putc('%');
            break;
        }
    }

    va_end(ap);
}

int init_logger(uint32_t width, uint32_t height)
{
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
    
    max_x = width / font_header.width;
    max_y = height / font_header.height;
    
    return 0;
}