#include "external/multiboot2.h"
#include "mem/mmap.h"
#include "mem/pfa.h"
#include "mem/paging.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/isr.h"
#include "cpu/tss.h"
#include "cpu/interrupts.h"
#include "utils/bitmap.h"
#include "utils/macros.h"
#include <stdint.h>
#include <mem.h>

void parse_multiboot_struct(uint64_t addr, uint64_t* out_size)
{
    uint64_t size = (uint64_t) *((uint32_t*) addr);
    struct multiboot_tag* tag;
    
    *out_size = size;

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
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
            break;
        }
    }
}

void kernel_main(uint64_t multiboot_struct_addr, bitmap_t* current_bitmap)
{
    uint64_t multiboot_struct_size;

    paging_init();
    pfa_restore(current_bitmap);
    parse_multiboot_struct(multiboot_struct_addr, &multiboot_struct_size);
    pfa_init();
    gdt_init(tss_init());
    idt_init();
    isr_init();
    enable_interrupts();

    __asm__ __volatile__ ("int $0x10");

    if (multiboot_struct_size == 0)
        goto HANG;
    
    HANG:
        while (1);
}