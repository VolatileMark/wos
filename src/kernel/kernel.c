#include "kernel.h"
#include "mem/mmap.h"
#include "mem/pfa.h"
#include "mem/heap.h"
#include "mem/paging.h"
#include "utils/macros.h"
#include "utils/helpers/log.h"
#include "utils/helpers/bitmap.h"
#include "utils/helpers/multiboot2-utils.h"
#include "sys/cpu/gdt.h"
#include "sys/cpu/idt.h"
#include "sys/cpu/isr.h"
#include "sys/cpu/tss.h"
#include "sys/cpu/interrupts.h"
#include "sys/chips/pit.h"
#include "sys/drivers/acpi.h"
#include "sys/drivers/pci.h"
#include "sys/drivers/video/framebuffer.h"
#include "proc/process.h"
#include "proc/scheduler.h"
#include "proc/syscall.h"
#include "proc/vfs/vfs.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <mem.h>

static int gather_system_info(void)
{
    if (init_acpi() || scan_pci())
        return -1;
    return 0;
}

static int kernel_init(uint64_t multiboot_struct_addr, bitmap_t* current_bitmap)
{
    init_paging();
    restore_pfa(current_bitmap);
    parse_multiboot_struct(multiboot_struct_addr);
    if (get_multiboot_struct_size() == 0)
        return -1;
    
    init_mmap(get_multiboot_tag_mmap());
    init_pfa();
    init_tss();
    init_gdt();
    init_idt();
    init_isr();

    init_framebuffer_driver();
    if (init_logger((get_framebuffer_width() * 3) / 4, get_framebuffer_height()))
        return -1;

    if (init_kernel_heap(KERNEL_HEAP_START_ADDR, KERNEL_HEAP_CEIL_ADDR, SIZE_4KB))
        return -1;
    
    init_syscalls();
    init_vfs();
    init_pit();
    
    if (init_scheduler())
        return -1;
    
    return gather_system_info();
}

void launch_init(void)
{
    /*
    delete_zombie_processes();
    schedule_runnable_process(create_process(&init_descriptor, 0));
    schedule_runnable_process(create_process(&fsrv_descriptor, 1));
    run_scheduler();
    */
}

void kernel_main(uint64_t multiboot_struct_addr, bitmap_t* current_bitmap)
{
    disable_interrupts();
    if (kernel_init(multiboot_struct_addr, current_bitmap))
    {
        error("Could not initialize kernel");
        HALT();
    }
    info("Free memory: %d kB | Used memory: %d kB", get_free_memory() >> 10, get_used_memory() >> 10);
    launch_init();
    HALT();
}