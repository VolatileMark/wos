#ifndef __KERNEL_H__
#define __KERNEL_H__

#include "utils/log.h"
#include "utils/multiboot2-utils.h"
#include "utils/crc32.h"
#include "utils/alloc.h"
#include "sys/mem/mmap.h"
#include "sys/mem/pfa.h"
#include "sys/mem/heap.h"
#include "sys/cpu/gdt.h"
#include "sys/cpu/idt.h"
#include "sys/cpu/tss.h"
#include "sys/cpu/interrupts.h"
#include "sys/chips/pit.h"
#include "sys/drivers/power/acpi.h"
#include "sys/drivers/bus/pci.h"
#include "sys/drivers/video/framebuffer.h"
#include "sys/drivers/storage/ahci.h"
#include "sys/drivers/fs/devfs.h"
#include "sys/drivers/fs/isofs.h"
#include "sys/drivers/fs/drivefs.h"
#include "proc/scheduler.h"
#include "proc/syscall.h"
#include <stddef.h>

#define KERNEL_FILE_PATH "/boot/wkernel.elf"
#define KERNEL_STACK_SIZE 32768
#define KERNEL_HEAP_START_ADDR VADDR_GET(256, 0, 0, 0)
#define KERNEL_HEAP_CEIL_ADDR VADDR_GET(511, 509, 511, 511)

extern uint64_t _start_addr;
extern uint64_t _end_addr;

#endif