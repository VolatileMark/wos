#ifndef __MULTIBOOT_H__
#define __MULTIBOOT_H__

#include "../external/multiboot2.h"
#include <stdint.h>

void parse_multiboot_struct(uint64_t addr);

uint64_t get_multiboot_struct_size(void);
struct multiboot_tag_mmap* get_multiboot_tag_mmap(void);
struct multiboot_tag_module* get_multiboot_tag_initrd(void);
struct multiboot_tag_framebuffer* get_multiboot_tag_framebuffer(void);
struct multiboot_tag_new_acpi* get_multiboot_tag_new_acpi(void);

#endif