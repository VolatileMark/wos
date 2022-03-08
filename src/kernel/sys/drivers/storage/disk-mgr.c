#include "disk-mgr.h"
#include "../fs/mbr.h"
#include "../fs/gpt.h"
#include "../../../utils/log.h"
#include "../../../utils/helpers/alloc.h"
#include <stdint.h>
#include <stddef.h>

#define trace_disk(msg, ...) trace("DKMG", msg, ##__VA_ARGS__)

typedef struct disk_list_entry
{
    struct disk_list_entry* next;   
    disk_t disk;
} disk_list_entry_t;

typedef struct
{
    disk_list_entry_t* head;
    disk_list_entry_t* tail;
} disk_list_t;

static disk_list_t disk_list;

static uint64_t get_next_disk_id(void)
{
    if (disk_list.tail == NULL)
        return 0;
    return (disk_list.tail->disk.id + 1);
}

static uint64_t get_partition_table(disk_t* disk)
{

    uint32_t partitions;
    if ((partitions = check_gpt(disk)));
    else if ((partitions = check_mbr(disk)));
    else 
        partitions = 1;
    return partitions;
}

void init_disk_manager(void)
{
    disk_list.head = NULL;
    disk_list.tail = NULL;
}

int register_disk(void* control, disk_ops_t* ops)
{
    disk_list_entry_t* entry;
    uint64_t num_partitions;

    entry = malloc(sizeof(disk_list_entry_t));
    if (entry == NULL)
    {
        trace_disk("Could not allocate space for new drive list entry");
        return -1;
    }

    entry->disk.id = get_next_disk_id();
    entry->disk.ops = ops;
    entry->disk.control = control;
    num_partitions = get_partition_table(&entry->disk);
    entry->next = NULL;

    if (disk_list.tail == NULL)
        disk_list.head = entry;
    else
        disk_list.tail->next = entry;
    disk_list.tail = entry;

    info("Registered new disk with id %u, it has %u partitions", entry->disk.id, num_partitions);

    return 0;
}