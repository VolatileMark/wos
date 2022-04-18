#include "tty.h"
#include "multiboot2-utils.h"
#include "psf.h"
#include "macros.h"
#include "log.h"
#include "../proc/vfs/vattribs.h"
#include "../sys/drivers/video/framebuffer.h"
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

extern volatile psf2_file_t _binary_font_psf_start;

static uint32_t palette[5];
static uint32_t cursor_x, cursor_y;
static uint32_t max_x, max_y;
static uint32_t offset_x, offset_y;
static uint32_t fg_color, bg_color;
static vnode_ops_t vnode_ops;
static framebuffer_info_t* fb;

#define font_header _binary_font_psf_start.header
#define font_glyphs _binary_font_psf_start.glyphs

static void tty_putc(char c)
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

static void tty_puts(const char* str)
{
    uint8_t c;
    while (*str != '\0')
    {
        if (*str != '@')
        {
            tty_putc(*str++);
            continue;
        }
    
        c = *(++str) - '0';
        if (c < (sizeof(palette) / sizeof(uint32_t)))
        {
            fg_color = palette[c];
            ++str;
        }
        else
            tty_putc('@');
    }
}

void tty_printf(const char* str, ...)
{
    va_list ap;
    char* p;
    char tmp_str[64];
    uint8_t i;

    va_start(ap, str);

    for (p = ((char*) str); *p != '\0'; p++)
    {
        if (*p == '@')
        {
            i = *(p + 1) - '0';
            if (i < (sizeof(palette) / sizeof(uint32_t)))
            {
                fg_color = palette[i];
                ++p;
            }
            else
                tty_putc(*p);
            continue;
        }
        else if (*p != '%')
        {
            tty_putc(*p);
            continue;
        }

        tmp_str[0] = '\0';
        switch (*(++p))
        {
        case 'c':
            tty_putc((uint8_t) va_arg(ap, uint32_t));
            break;
        case 'p':
            tty_puts(utoan(va_arg(ap, uint64_t), tmp_str, 16, 16));
            break;
        case 'x':
            tty_puts(itoa(va_arg(ap, long), tmp_str, 16));
            break;
        case 'd':
        case 'i':
            tty_puts(itoa(va_arg(ap, long), tmp_str, 10));
            break;
        case 'f':
            break;
        case 's':
            tty_puts(va_arg(ap, char*));
            break;
        case 'u':
            tty_puts(utoa(va_arg(ap, uint64_t), tmp_str, 10));
            break;
        case '%':
            tty_putc('%');
            break;
        }
    }

    va_end(ap);
}

void tty_clear_screen(void)
{
    uint32_t* fb_ptr;
    uint64_t i;

    fb_ptr = (uint32_t*) fb->addr;
    for (i = 0; i < fb->size; i++)
        fb_ptr[i] = bg_color;
}

static int tty_open(vnode_t* vnode)
{
    UNUSED(vnode);
    return 0;
}

static int tty_lookup(vnode_t* vnode, const char* path, vnode_t* out)
{
    UNUSED(vnode);
    UNUSED(path);
    UNUSED(out);
    return -1;
}

static int tty_read(vnode_t* vnode, void* buffer, uint64_t count)
{
    UNUSED(vnode);
    UNUSED(buffer);
    UNUSED(count);
    return -1;
}

static int tty_write(vnode_t* vnode, const char* data, uint64_t count)
{
    UNUSED(vnode);
    UNUSED(count);
    tty_puts(data);
    return 0;
}

static int tty_get_attribs(vnode_t* vnode, vattribs_t* attr)
{
    UNUSED(vnode);
    attr->size = fb->size;
    return 0;
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

    palette[0] = framebuffer_color(255, 255, 255);
    palette[1] = framebuffer_color(145, 25, 0);
    palette[2] = framebuffer_color(225, 225, 0);
    palette[3] = framebuffer_color(10, 128, 10);
    palette[4] = framebuffer_color(0, 0, 0);

    fg_color = palette[0];
    bg_color = palette[4];

    vnode_ops.open = &tty_open;
    vnode_ops.lookup = &tty_lookup;
    vnode_ops.read = &tty_read;
    vnode_ops.write = &tty_write;
    vnode_ops.get_attribs = &tty_get_attribs;

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

void tty_set_background_color(uint8_t r, uint8_t g, uint8_t b)
{
    bg_color = framebuffer_color(r, g, b);
}

void tty_set_foreground_color(uint8_t r, uint8_t g, uint8_t b)
{
    fg_color = framebuffer_color(r, g, b);
}

int tty_get_vnode(vnode_t* out)
{
    out->ops = &vnode_ops;
    out->data = NULL;
    return 0;
}