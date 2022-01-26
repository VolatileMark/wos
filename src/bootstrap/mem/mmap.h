#ifndef __MMAP_H__
#define __MMAP_H__

#include "../external/multiboot2.h"
#include <stdint.h>

int init_mmap(struct multiboot_tag_mmap* tag);
struct multiboot_mmap_entry* get_largest_mmap_entry_of_type(uint64_t type);
uint64_t get_total_memory_of_type(uint64_t type);

#endif