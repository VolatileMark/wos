#include "mmap.h"
#include <stddef.h>

static uint64_t memory_segment_sizes[5];
static struct multiboot_mmap_entry* largest_segments[5];

void mmap_init(struct multiboot_tag_mmap* tag)
{
    memory_segment_sizes[MULTIBOOT_MEMORY_BADRAM] = 0;
    memory_segment_sizes[MULTIBOOT_MEMORY_NVS] = 0;
    memory_segment_sizes[MULTIBOOT_MEMORY_ACPI_RECLAIMABLE] = 0;
    memory_segment_sizes[MULTIBOOT_MEMORY_RESERVED] = 0;
    memory_segment_sizes[MULTIBOOT_MEMORY_AVAILABLE] = 0;

    largest_segments[MULTIBOOT_MEMORY_BADRAM] = NULL;
    largest_segments[MULTIBOOT_MEMORY_NVS] = NULL;
    largest_segments[MULTIBOOT_MEMORY_ACPI_RECLAIMABLE] = NULL;
    largest_segments[MULTIBOOT_MEMORY_RESERVED] = NULL;
    largest_segments[MULTIBOOT_MEMORY_AVAILABLE] = NULL;

    uint32_t entries = (tag->size - (((uint64_t) tag) - ((uint64_t) &tag->entries))) / tag->entry_size;
    uint32_t i;
    struct multiboot_mmap_entry* entry;
    for (i = 0; i < entries; i ++)
    {
        entry = &tag->entries[i];
        memory_segment_sizes[entry->type] += entry->len;
        if 
        (
            largest_segments[entry->type] != NULL &&
            largest_segments[entry->type]->len < entry->len
        )
            largest_segments[entry->type] = entry;
    }
}

struct multiboot_mmap_entry* get_largest_mmap_entry_of_type(uint64_t type)
{
    return largest_segments[type];
}

uint64_t get_total_memory_of_type(uint64_t type)
{
    return memory_segment_sizes[type];
}