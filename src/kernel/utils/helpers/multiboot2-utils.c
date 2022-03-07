#include "multiboot2-utils.h"
#include "../../mem/paging.h"
#include "../macros.h"
#include <mem.h>
#include <stddef.h>

#define trace_multiboot(msg, ...) trace("MULB", msg, ##__VA_ARGS__)

static uint64_t struct_size;
static struct multiboot_tag_mmap* mmap;
static struct multiboot_tag_module* module;
static struct multiboot_tag_framebuffer* framebuffer;
static struct multiboot_tag_new_acpi* new_acpi;
static struct multiboot_tag_old_acpi* old_acpi;

int remap_struct(uint64_t struct_paddr)
{
    uint64_t struct_vaddr;

#define REMAP_TAG(tag) tag = (struct multiboot_tag_##tag*) (struct_vaddr + (((uint64_t) tag) - struct_paddr))

    kernel_unmap_memory(0, SIZE_nMB(16));
    if (kernel_get_next_vaddr(struct_size, &struct_vaddr) < struct_size)
        return -1;
    if (kernel_map_memory(struct_paddr, struct_vaddr, struct_size, PAGE_ACCESS_RO, PL0) < struct_size)
    {
        kernel_unmap_memory(struct_vaddr, struct_size);
        return -1;
    }

    REMAP_TAG(mmap);
    REMAP_TAG(module);
    REMAP_TAG(framebuffer);
    REMAP_TAG(old_acpi);
    if (new_acpi != NULL)
        REMAP_TAG(new_acpi);

    return 0;
}

int parse_multiboot_struct(uint64_t addr)
{
    struct multiboot_tag* tag;

    struct_size = (uint64_t) *((uint32_t*) addr);
    if (struct_size == 0)
        return -1;

    mmap = NULL;
    module = NULL;
    framebuffer = NULL;
    old_acpi = NULL;
    new_acpi = NULL;

    for 
    (
        tag = (struct multiboot_tag*) alignu(addr + sizeof(struct multiboot_tag), MULTIBOOT_TAG_ALIGN);
        ((uint64_t) tag) < (addr + struct_size) && tag->type != MULTIBOOT_TAG_TYPE_END;
        tag = (struct multiboot_tag*) alignu(((uint64_t) tag) + tag->size, MULTIBOOT_TAG_ALIGN)
    )
    {
        switch (tag->type)
        {
        case MULTIBOOT_TAG_TYPE_MMAP:
            mmap = (struct multiboot_tag_mmap*) tag;
            break;
        case MULTIBOOT_TAG_TYPE_MODULE:
            module = (struct multiboot_tag_module*) tag;
            break;
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
            framebuffer = (struct multiboot_tag_framebuffer*) tag;
            break;
        case MULTIBOOT_TAG_TYPE_ACPI_NEW:
            new_acpi = (struct multiboot_tag_new_acpi*) tag;
            break;
        case MULTIBOOT_TAG_TYPE_ACPI_OLD:
            old_acpi = (struct multiboot_tag_old_acpi*) tag;
            break;
        }
    }

    if 
    (
        mmap == NULL ||
        module == NULL ||
        framebuffer == NULL ||
        old_acpi == NULL
    )
        return -1;

    return 0;
}

uint64_t get_multiboot_struct_size(void)
{
    return struct_size;
}

struct multiboot_tag_mmap* get_multiboot_tag_mmap(void)
{
    return mmap;
}
struct multiboot_tag_module* get_multiboot_tag_module(void)
{
    return module;
}

struct multiboot_tag_framebuffer* get_multiboot_tag_framebuffer(void)
{
    return framebuffer;
}

struct multiboot_tag_new_acpi* get_multiboot_tag_new_acpi(void)
{
    return new_acpi;
}

struct multiboot_tag_old_acpi* get_multiboot_tag_old_acpi(void)
{
    return old_acpi;
}
