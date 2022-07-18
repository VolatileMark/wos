#ifndef __PSF_H__
#define __PSF_H__

#include <stdint.h>

#define PSF2_FONT_MAGIC 0x864AB572

struct psf2_header
{
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t flags;
    uint32_t num_glyphs;
    uint32_t bytes_per_glyph;
    uint32_t height;
    uint32_t width;
} __attribute__((packed));
typedef struct psf2_header psf2_header_t;

struct psf2_file
{
    psf2_header_t header;
    uint8_t glyphs[];
} __attribute__((packed));
typedef struct psf2_file psf2_file_t;


#endif
