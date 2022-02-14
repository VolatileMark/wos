#include "multiboot2-utils.h"
#include <mem.h>
#include <stddef.h>

static uint64_t struct_size;
static struct multiboot_tag_mmap* mmap;
static struct multiboot_tag_module* initrd;
static struct multiboot_tag_framebuffer* framebuffer;
static struct multiboot_tag_new_acpi* new_acpi;
static struct multiboot_tag_old_acpi* old_acpi;

void parse_multiboot_struct(uint64_t addr)
{
    struct multiboot_tag* tag;

    struct_size = (uint64_t) *((uint32_t*) addr);
    mmap = NULL;
    initrd = NULL;
    framebuffer = NULL;
    new_acpi = NULL;
    old_acpi = NULL;

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
            initrd = (struct multiboot_tag_module*) tag;
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
}

uint64_t get_multiboot_struct_size(void)
{
    return struct_size;
}

struct multiboot_tag_mmap* get_multiboot_tag_mmap(void)
{
    return mmap;
}
struct multiboot_tag_module* get_multiboot_tag_initrd(void)
{
    return initrd;
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