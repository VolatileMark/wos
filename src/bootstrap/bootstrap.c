#include <stdint.h>
#include <mem.h>
#include "external/multiboot2.h"
#include "mem/mmap.h"
#include "mem/pfa.h"

void parse_multiboot2_struct(uint64_t addr)
{
    uint64_t size = (uint64_t) *((uint32_t*) addr);
    struct multiboot_tag* tag;

    for 
    (
        tag = (struct multiboot_tag*) alignu(addr + sizeof(struct multiboot_tag), MULTIBOOT_TAG_ALIGN);
        ((uint64_t) tag) < (addr + size) && tag->type != MULTIBOOT_TAG_TYPE_END;
        tag = (struct multiboot_tag*) alignu(((uint64_t) tag) + tag->size, MULTIBOOT_TAG_ALIGN)
    )
    {
        switch (tag->type)
        {
        case MULTIBOOT_TAG_TYPE_MMAP:
            mmap_init((struct multiboot_tag_mmap*) tag);
            break;
        case MULTIBOOT_TAG_TYPE_MODULE:
            break;
        }
    }
}

void bootstrap_main(uint64_t multiboot2_magic, uint64_t multiboot2_struct_addr)
{
    if 
    (
        multiboot2_magic != MULTIBOOT2_BOOTLOADER_MAGIC /* Check if the magic number is correct */ ||
        multiboot2_struct_addr & 0x0000000000000007 /* Check if the info struct is aligned to 4KB */
    )
        goto FAIL;
    
    parse_multiboot2_struct(multiboot2_struct_addr);
    
    pfa_init();

    FAIL:
        while (1);
}