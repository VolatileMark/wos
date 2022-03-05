#include "kernel.h"
#include "sys/drivers/storage/disk-mgr.h"

static int init_kernel(uint64_t multiboot_struct_addr, bitmap_t* current_bitmap)
{
    disable_interrupts();

    init_paging();
    restore_pfa(current_bitmap);
    if (parse_multiboot_struct(multiboot_struct_addr))
        return -1;
    
    init_mmap(get_multiboot_tag_mmap());
    init_pfa();
    init_tss();
    init_gdt();
    init_idt();
    init_isr();

    if 
    (
        remap_struct(multiboot_struct_addr) ||
        init_framebuffer_driver() ||
        init_screen()
    )
        return -1;

    if (init_kernel_heap(KERNEL_HEAP_START_ADDR, KERNEL_HEAP_CEIL_ADDR, SIZE_4KB))
        return -1;
    
    init_syscalls();
    init_disk_manager();
    init_vfs();
    init_pit();

    if 
    (
        init_scheduler() || 
        init_acpi() || 
        scan_pci() || 
        init_ahci_driver()
    )
        return -1;
    
    return 0;
}

void kernel_main(uint64_t multiboot_struct_addr, bitmap_t* current_bitmap)
{
    if (init_kernel(multiboot_struct_addr, current_bitmap))
    {
        if (is_screen_initialized())
            error("Could not initialize kernel");
        HALT();
    }
    info("Kernel initialized (FREE: %u kB | USED: %u kB)", get_free_memory() >> 10, get_used_memory() >> 10);
    HALT();
}