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

static void gather_system_info(void)
{
    if (init_acpi())
        HALT();
    scan_pci();
}

static void kernel_init(uint64_t multiboot_struct_addr, bitmap_t* current_bitmap)
{
    init_paging();
    restore_pfa(current_bitmap);
    parse_multiboot_struct(multiboot_struct_addr);
    if (get_multiboot_struct_size() == 0)
        HALT();
    
    init_mmap(get_multiboot_tag_mmap());
    init_pfa();
    init_tss();
    init_gdt();
    init_idt();
    init_isr();

    init_framebuffer_driver();

    init_kernel_heap(KERNEL_HEAP_START_ADDR, KERNEL_HEAP_CEIL_ADDR, SIZE_4KB);
    init_syscalls();
    init_pit();
    init_scheduler();
    init_vfs();

    gather_system_info();

    if (init_logger(800, 768))
        HALT();
}

void launch_init(void)
{
    /*
    delete_zombie_processes();
    schedule_runnable_process(create_process(&init_descriptor, 0));
    schedule_runnable_process(create_process(&fsrv_descriptor, 1));
    run_scheduler();
    */
    trace("KERNEL", "This is a test trace %d", 1234);
    error("This is a test error %u", -1);
    warning("This is a test warning %p", 0x4000);
}

void kernel_main(uint64_t multiboot_struct_addr, bitmap_t* current_bitmap)
{
    disable_interrupts();
    kernel_init(multiboot_struct_addr, current_bitmap);
    launch_init();
    HALT();
}