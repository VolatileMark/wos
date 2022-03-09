#ifndef __MULTIBOOT2_UTILS_H__
#define __MULTIBOOT2_UTILS_H__

#include "multiboot2.h"
#include <stdint.h>

int multiboot_parse_struct(uint64_t addr);
int multiboot_remap_struct(uint64_t struct_paddr);

uint64_t get_multiboot_struct_size(void);
struct multiboot_tag_mmap* multiboot_get_tag_mmap(void);
struct multiboot_tag_module* multiboot_get_tag_module(void);
struct multiboot_tag_framebuffer* multiboot_get_tag_framebuffer(void);
struct multiboot_tag_new_acpi* multiboot_get_tag_new_acpi(void);
struct multiboot_tag_old_acpi* multiboot_get_tag_old_acpi(void);

#endif