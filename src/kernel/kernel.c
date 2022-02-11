#include "kernel.h"
#include "mem/mmap.h"
#include "mem/pfa.h"
#include "mem/heap.h"
#include "mem/paging.h"
#include "utils/macros.h"
#include "utils/helpers/bitmap.h"
#include "utils/helpers/mb2utils.h"
#include "sys/cpu/gdt.h"
#include "sys/cpu/idt.h"
#include "sys/cpu/isr.h"
#include "sys/cpu/tss.h"
#include "sys/cpu/interrupts.h"
#include "sys/chips/pit.h"
#include "sys/acpi.h"
#include "sys/pci.h"
#include "proc/process.h"
#include "proc/scheduler.h"
#include "proc/syscall.h"
#include "proc/ipc/spp.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <mem.h>

static process_descriptor_t init_descriptor, fsrv_descriptor;

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

    init_kernel_heap(KERNEL_HEAP_START_ADDR, KERNEL_HEAP_CEIL_ADDR, SIZE_4KB);
    init_syscalls();
    init_pit();
    init_scheduler();
    init_spp();

    gather_system_info();
}

void launch_init(void)
{
    delete_zombie_processes();
    schedule_runnable_process(create_process(&init_descriptor, 0));
    schedule_runnable_process(create_process(&fsrv_descriptor, 1));
    run_scheduler();
}

void kernel_main
(
    uint64_t multiboot_struct_addr, 
    uint64_t init_exec_file_paddr, uint64_t init_exec_file_size, 
    uint64_t fsrv_exec_file_paddr, uint64_t fsrv_exec_file_size, 
    bitmap_t* current_bitmap
)
{
    disable_interrupts();
    kernel_init(multiboot_struct_addr, current_bitmap);

    init_descriptor.exec_paddr = init_exec_file_paddr;
    init_descriptor.exec_size = init_exec_file_size;
    init_descriptor.exec_type = PROC_EXEC_ELF;
    init_descriptor.cmdline = NULL;

    fsrv_descriptor.exec_paddr = fsrv_exec_file_paddr;
    fsrv_descriptor.exec_size = fsrv_exec_file_size;
    fsrv_descriptor.exec_type = PROC_EXEC_ELF;
    fsrv_descriptor.cmdline = NULL;

    launch_init();
}