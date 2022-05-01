#include "kernel.h"

static int kernel_init_fs(void)
{
    vfs_t* devfs;
    vnode_t* vnode;

    vfs_init();
    devfs_init();
    drivefs_init();
    isofs_init();
    
    devfs = malloc(sizeof(vfs_t));
    if (devfs == NULL)
    {
        error("Failed to allocate memory for devfs");
        return -1;
    }
    else if (devfs_create(devfs))
    {
        free(devfs);
        error("Could not initialize devfs");
        return -1;
    }
    else if (vfs_mount("/dev", devfs))
    {
        free(devfs);
        error("Could not mount devfs");
        return -1;
    }
    
    vnode = malloc(sizeof(vnode_t));
    if (vnode == NULL)
    {
        error("Could not allocate vnode for tty0");
        return -1;
    }
    tty_get_vnode(vnode);
    if (devfs_add_device("tty0", vnode))
    {
        free(vnode);
        error("Could not add tty0 to devfs");
        return -1;
    }

    return 0;
}

static int kernel_init(uint64_t multiboot_struct_addr, bitmap_t* current_bitmap)
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
    
    pit_init();
    syscall_init();

    if (kernel_init_fs())
        return -1;

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
    if (kernel_init(multiboot_struct_addr, current_bitmap))
    {
        if (tty_is_initialized())
            error("Could not initialize kernel");
        return;
    }
    info("Kernel initialized (FREE: %u kB | USED: %u kB)", pfa_get_free_memory() >> 10, pfa_get_used_memory() >> 10);
}