#include "multiboot2-utils.h"
#include "../../mem/paging.h"
#include "../macros.h"
#include "log.h"
#include <mem.h>
#include <stddef.h>

#define trace_multiboot(msg, ...) trace("MULB", msg, ##__VA_ARGS__)

static uint64_t struct_size;
static struct multiboot_tag_mmap* mmap;
static struct multiboot_tag_module* kernel_elf;
static struct multiboot_tag_framebuffer* framebuffer;
static struct multiboot_tag_new_acpi* new_acpi;
static struct multiboot_tag_old_acpi* old_acpi;

int remap_struct(uint64_t struct_paddr)
{
    uint64_t struct_vaddr;
    kernel_unmap_memory(0, SIZE_nMB(16));
    if (kernel_get_next_vaddr(struct_size, &struct_vaddr) < struct_size)
    {
        trace_multiboot("Could not get vaddr for multiboot struct");
        return -1;
    }
    if (kernel_map_memory(struct_paddr, struct_vaddr, struct_size, PAGE_ACCESS_RO, PL0) < struct_size)
    {
        trace_multiboot("Could not map multiboot struct");
        return -1;
    }

    mmap = (struct multiboot_tag_mmap*) (struct_vaddr + (((uint64_t) mmap) - struct_paddr));
    kernel_elf = (struct multiboot_tag_module*) (struct_vaddr + (((uint64_t) kernel_elf) - struct_paddr));
    framebuffer = (struct multiboot_tag_framebuffer*) (struct_vaddr + (((uint64_t) framebuffer) - struct_paddr));
    new_acpi = (struct multiboot_tag_new_acpi*) (struct_vaddr + (((uint64_t) new_acpi) - struct_paddr));
    old_acpi = (struct multiboot_tag_old_acpi*) (struct_vaddr + (((uint64_t) old_acpi) - struct_paddr));

    return 0;
}

int parse_multiboot_struct(uint64_t addr)
{
    struct multiboot_tag* tag;

    struct_size = (uint64_t) *((uint32_t*) addr);
    if (struct_size == 0)
    {
        trace_multiboot("Multiboot structure size is 0");
        return -1;
    }

    mmap = NULL;
    kernel_elf = NULL;
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
            kernel_elf = (struct multiboot_tag_module*) tag;
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
struct multiboot_tag_module* get_multiboot_tag_initrd(void)
{
    return kernel_elf;
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