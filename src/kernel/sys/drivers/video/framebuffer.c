#include "framebuffer.h"
#include "../../mem/paging.h"
#include "../../mem/pfa.h"
#include "../../../utils/multiboot2-utils.h"
#include <math.h>

#define FRAMEBUFFER_DIRECT_COLOR 1

static framebuffer_info_t framebuffer_info;

static uint32_t get_mask(uint8_t size, uint8_t offset, uint32_t* out_offset)
{
    uint32_t mask;
    for (mask = 0; size > 0; size--)
    {
        mask <<= 1;
        mask |= 1;
    }
    *out_offset = offset;
    return ~(mask << offset);
}

int framebuffer_init(void)
{
    struct multiboot_tag_framebuffer* framebuffer_tag;

    framebuffer_tag = multiboot_get_tag_framebuffer();
    if (framebuffer_tag->common.framebuffer_type != FRAMEBUFFER_DIRECT_COLOR)
        return -1;
    
    framebuffer_info.width = framebuffer_tag->common.framebuffer_width;
    framebuffer_info.height = framebuffer_tag->common.framebuffer_height;
    framebuffer_info.pitch = framebuffer_tag->common.framebuffer_pitch;
    framebuffer_info.bytes_per_pixel = framebuffer_tag->common.framebuffer_bpp / 8;
    framebuffer_info.size = framebuffer_info.pitch * framebuffer_info.height;
    pfa_lock_pages(framebuffer_tag->common.framebuffer_addr, ceil((double) framebuffer_info.size / SIZE_4KB));

    if (kernel_get_next_vaddr(framebuffer_info.size, &framebuffer_info.addr) < framebuffer_info.size)
        return -1;

    if (paging_map_memory(framebuffer_tag->common.framebuffer_addr, framebuffer_info.addr, framebuffer_info.size, PAGE_ACCESS_RW, PL0) < framebuffer_info.size)
    {
        paging_unmap_memory(framebuffer_info.addr, framebuffer_info.size);
        return -1;
    }

    framebuffer_info.red_mask = get_mask(framebuffer_tag->framebuffer_red_mask_size, framebuffer_tag->framebuffer_red_field_position, &framebuffer_info.red_offset);
    framebuffer_info.green_mask = get_mask(framebuffer_tag->framebuffer_green_mask_size, framebuffer_tag->framebuffer_green_field_position, &framebuffer_info.green_offset);
    framebuffer_info.blue_mask = get_mask(framebuffer_tag->framebuffer_blue_mask_size, framebuffer_tag->framebuffer_blue_field_position, &framebuffer_info.blue_offset);

    return 0;
}

inline int put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    uint32_t* pixel_data;
    
    if (x > framebuffer_info.width || y > framebuffer_info.height)
        return -1;
    
    pixel_data = (uint32_t*) (framebuffer_info.addr + ((x * framebuffer_info.bytes_per_pixel) + (y * framebuffer_info.pitch)));
    *pixel_data = color;

    return 0;
}

framebuffer_info_t* framebuffer_get(void)
{
    return &framebuffer_info;
}

uint32_t framebuffer_width(void)
{
    return framebuffer_info.width;
}

uint32_t framebuffer_height(void)
{
    return framebuffer_info.height;
}

uint64_t framebuffer_vaddr(void)
{
    return framebuffer_info.addr;
}

uint64_t framebuffer_size(void)
{
    return framebuffer_info.size;
}

uint32_t framebuffer_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t color;
    color = 0;
    color = (color & framebuffer_info.red_mask) | (((uint32_t) r) << framebuffer_info.red_offset);
    color = (color & framebuffer_info.green_mask) | (((uint32_t) g) << framebuffer_info.green_offset);
    color = (color & framebuffer_info.blue_mask) | (((uint32_t) b) << framebuffer_info.blue_offset);
    return color;
}