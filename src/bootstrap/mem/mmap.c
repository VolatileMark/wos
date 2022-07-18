#include "mmap.h"
#include "../utils/constants.h"
#include <stddef.h>

static uint64_t memory_segment_sizes[5];
static struct multiboot_mmap_entry* largest_segments[5];

int mmap_init(struct multiboot_tag_mmap* tag)
{
    struct multiboot_mmap_entry* entry;
    uint32_t i, entries;

    entries = (tag->size - (((uint64_t) tag) - ((uint64_t) &tag->entries))) / tag->entry_size;

    memory_segment_sizes[MULTIBOOT_MEMORY_BADRAM - 1] = 0;
    memory_segment_sizes[MULTIBOOT_MEMORY_NVS - 1] = 0;
    memory_segment_sizes[MULTIBOOT_MEMORY_ACPI_RECLAIMABLE - 1] = 0;
    memory_segment_sizes[MULTIBOOT_MEMORY_RESERVED - 1] = 0;
    memory_segment_sizes[MULTIBOOT_MEMORY_AVAILABLE - 1] = 0;

    largest_segments[MULTIBOOT_MEMORY_BADRAM - 1] = NULL;
    largest_segments[MULTIBOOT_MEMORY_NVS - 1] = NULL;
    largest_segments[MULTIBOOT_MEMORY_ACPI_RECLAIMABLE - 1] = NULL;
    largest_segments[MULTIBOOT_MEMORY_RESERVED - 1] = NULL;
    largest_segments[MULTIBOOT_MEMORY_AVAILABLE - 1] = NULL;

    for (i = 0; i < entries; i ++)
    {
        entry = &tag->entries[i];
        memory_segment_sizes[entry->type - 1] += entry->len;
        if 
        (
            largest_segments[entry->type - 1] != NULL &&
            largest_segments[entry->type - 1]->len < entry->len
        ) { largest_segments[entry->type - 1] = entry; }
    }

    if (memory_segment_sizes[MULTIBOOT_MEMORY_AVAILABLE - 1] < MIN_MEMORY) { return -1; }
    return 0;
}

struct multiboot_mmap_entry* mmap_get_largest_entry_of_type(uint64_t type)
{
    return largest_segments[type - 1];
}

uint64_t mmap_get_total_memory_of_type(uint64_t type)
{
    return memory_segment_sizes[type - 1];
}
