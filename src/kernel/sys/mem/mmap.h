#ifndef __MMAP_H__
#define __MMAP_H__

#include "../../headers/multiboot2.h"
#include <stdint.h>

void mmap_init(struct multiboot_tag_mmap* tag);
struct multiboot_mmap_entry* mmap_get_largest_entry_of_type(uint64_t type);
uint64_t mmap_get_total_memory_of_type(uint64_t type);

#endif