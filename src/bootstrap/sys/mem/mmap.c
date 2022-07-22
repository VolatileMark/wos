#include "mmap.h"
#include "../../utils/constants.h"
#include <stddef.h>

static uint64_t memory_segment_sizes[5];
static struct multiboot_mmap_entry* largest_segments[5];

int mmap_init(struct multiboot_tag_mmap* tag)
{
    uint32_t entry_type;
    struct multiboot_mmap_entry* entry;

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

    for (entry = tag->entries; ((uint8_t*) entry) < ((uint8_t*) tag) + tag->size; entry = (struct multiboot_mmap_entry*) (((uint64_t) entry) + tag->entry_size))
    {
        entry_type = entry->type - 1;
        memory_segment_sizes[entry_type] += entry->len;
        if 
        (
            largest_segments[entry_type] != NULL &&
            largest_segments[entry_type]->len < entry->len
        )
            largest_segments[entry_type] = entry;
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
