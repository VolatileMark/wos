#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "mem/paging.h"
#include "mem/mmap.h"
#include "mem/pfa.h"
#include "mem/heap.h"
#include "utils/macros.h"
#include "utils/log.h"
#include "utils/tty.h"
#include "utils/bitmap.h"
#include "utils/multiboot2-utils.h"
#include "utils/crc32.h"
#include "utils/alloc.h"
#include "sys/cpu/gdt.h"
#include "sys/cpu/idt.h"
#include "sys/cpu/isr.h"
#include "sys/cpu/tss.h"
#include "sys/cpu/interrupts.h"
#include "sys/chips/pit.h"
#include "sys/drivers/power/acpi.h"
#include "sys/drivers/bus/pci.h"
#include "sys/drivers/video/framebuffer.h"
#include "sys/drivers/storage/ahci.h"
#include "sys/drivers/fs/devfs.h"
#include "sys/drivers/fs/drivefs.h"
#include "proc/process.h"
#include "proc/scheduler.h"
#include "proc/syscall.h"
#include "proc/vfs/vfs.h"
#include <stdint.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <mem.h>

#define KERNEL_STACK_SIZE SIZE_4KB
#define KERNEL_HEAP_START_ADDR VADDR_GET(384, 0, 0, 0)
#define KERNEL_HEAP_CEIL_ADDR VADDR_GET(511, 509, 511, 511)

extern uint64_t _start_addr;
extern uint64_t _end_addr;

#endif