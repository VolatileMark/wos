#include "kernel.h"

static int init_kernel(uint64_t multiboot_struct_addr, bitmap_t* current_bitmap)
{
    interrupts_disable();

    paging_init();
    pfa_restore(current_bitmap);
    if (multiboot_parse_struct(multiboot_struct_addr))
        return -1;
    
    mmap_init(multiboot_get_tag_mmap());
    pfa_init();
    tss_init();
    gdt_init();
    init_idt();
    isr_init();
    
    crc32_fill_lookup_table();

    if 
    (
        multiboot_remap_struct(multiboot_struct_addr) ||
        framebuffer_init() ||
        tty_init()
    )
        return -1;

    if (heap_init(KERNEL_HEAP_START_ADDR, KERNEL_HEAP_CEIL_ADDR, SIZE_4KB))
        return -1;
    
    syscall_init();
    vfs_init();
    pit_init();

    if 
    (
        scheduler_init() || 
        acpi_init() || 
        pci_init() || 
        ahci_init()
    )
        return -1;
    
    return 0;
}

void kernel_main(uint64_t multiboot_struct_addr, bitmap_t* current_bitmap)
{
    if (init_kernel(multiboot_struct_addr, current_bitmap))
    {
        if (tty_is_initialized())
            error("Could not initialize kernel");
        return;
    }
    info("Kernel initialized (FREE: %u kB | USED: %u kB)", pfa_get_free_memory() >> 10, pfa_get_used_memory() >> 10);
}