#include "framebuffer.h"
#include "../../../mem/paging.h"
#include "../../../utils/helpers/log.h"
#include "../../../utils/helpers/multiboot2-utils.h"

#define FRAMEBUFFER_DIRECT_COLOR 1

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
    uint64_t checksum;
} framebuffer_info_t;

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

int init_framebuffer_driver(void)
{
    uint64_t i;
    struct multiboot_tag_framebuffer* framebuffer_tag;

    framebuffer_info.checksum = 0;

    framebuffer_tag = get_multiboot_tag_framebuffer();
    if (framebuffer_tag->common.framebuffer_type != FRAMEBUFFER_DIRECT_COLOR)
        return -1;
    
    framebuffer_info.width = framebuffer_tag->common.framebuffer_width;
    framebuffer_info.height = framebuffer_tag->common.framebuffer_height;
    framebuffer_info.pitch = framebuffer_tag->common.framebuffer_pitch;
    framebuffer_info.bytes_per_pixel = framebuffer_tag->common.framebuffer_bpp / 8;
    framebuffer_info.size = framebuffer_info.pitch * framebuffer_info.height;

    if (kernel_get_next_vaddr(framebuffer_info.size, &framebuffer_info.addr) < framebuffer_info.size)
        return -1;

    if (kernel_map_memory(framebuffer_tag->common.framebuffer_addr, framebuffer_info.addr, framebuffer_info.size, PAGE_ACCESS_RW, PL0) < framebuffer_info.size)
    {
        kernel_unmap_memory(framebuffer_info.addr, framebuffer_info.size);
        return -1;
    }

    framebuffer_info.red_mask = get_mask(framebuffer_tag->framebuffer_red_mask_size, framebuffer_tag->framebuffer_red_field_position, &framebuffer_info.red_offset);
    framebuffer_info.green_mask = get_mask(framebuffer_tag->framebuffer_green_mask_size, framebuffer_tag->framebuffer_green_field_position, &framebuffer_info.green_offset);
    framebuffer_info.blue_mask = get_mask(framebuffer_tag->framebuffer_blue_mask_size, framebuffer_tag->framebuffer_blue_field_position, &framebuffer_info.blue_offset);

    for (i = 0; i < sizeof(framebuffer_info_t) - 1; i++)
        framebuffer_info.checksum -= ((uint8_t*) &framebuffer_info)[i];

    return 0;
}

inline int put_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t* pixel_data;
    
    if (x > framebuffer_info.width || y > framebuffer_info.height)
        return -1;
    
    pixel_data = (uint32_t*) (framebuffer_info.addr + ((x * framebuffer_info.bytes_per_pixel) + (y * framebuffer_info.pitch)));
    *pixel_data = (*pixel_data & framebuffer_info.red_mask) | (((uint32_t) r) << framebuffer_info.red_offset);
    *pixel_data = (*pixel_data & framebuffer_info.green_mask) | (((uint32_t) g) << framebuffer_info.green_offset);
    *pixel_data = (*pixel_data & framebuffer_info.blue_mask) | (((uint32_t) b) << framebuffer_info.blue_offset);

    return 0;
}

uint32_t get_framebuffer_width(void)
{
    return framebuffer_info.width;
}

uint32_t get_framebuffer_height(void)
{
    return framebuffer_info.height;
}

uint64_t get_framebuffer_vaddr(void)
{
    return framebuffer_info.addr;
}

int is_framebuffer_driver_initialized(void)
{
    uint64_t i, sum;
    for (i = 0, sum = 0; i < sizeof(framebuffer_info_t); i++)
        sum += ((uint8_t*) &framebuffer_info)[i];
    return (sum == 0);
}