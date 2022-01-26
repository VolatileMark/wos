#include "mem/mmap.h"
#include "mem/pfa.h"
#include "mem/kheap.h"
#include "mem/paging.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "cpu/isr.h"
#include "cpu/tss.h"
#include "cpu/interrupts.h"
#include "utils/bitmap.h"
#include "utils/macros.h"
#include "utils/multiboot.h"
#include "drivers/chips/pit.h"
#include "sys/process.h"
#include "sys/scheduler.h"
#include "sys/syscall.h"
#include <stdint.h>
#include <mem.h>

void kernel_main
(
    uint64_t multiboot_struct_addr, 
    uint64_t init_exec_file_paddr, 
    uint64_t init_exec_file_size, 
    bitmap_t* current_bitmap
)
{
    disable_interrupts();

    init_paging();
    restore_pfa(current_bitmap);
    parse_multiboot_struct(multiboot_struct_addr);
    if (get_multiboot_struct_size() == 0)
        goto HANG;
    
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

    set_init_exec_file(init_exec_file_paddr, init_exec_file_size);
    run_scheduler();
    
    HANG:
        while (1)
            __asm__("hlt");
}