#ifndef __MULTIBOOT2_UTILS_H__
#define __MULTIBOOT2_UTILS_H__

#include "../multiboot2.h"
#include <stdint.h>

int parse_multiboot_struct(uint64_t addr);
int remap_struct(uint64_t struct_paddr);

uint64_t get_multiboot_struct_size(void);
struct multiboot_tag_mmap* get_multiboot_tag_mmap(void);
struct multiboot_tag_module* get_multiboot_tag_initrd(void);
struct multiboot_tag_framebuffer* get_multiboot_tag_framebuffer(void);
struct multiboot_tag_new_acpi* get_multiboot_tag_new_acpi(void);
struct multiboot_tag_old_acpi* get_multiboot_tag_old_acpi(void);

#endif