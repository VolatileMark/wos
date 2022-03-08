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

static partition_table_type_t get_partition_table_type(disk_t* disk)
{
    if (check_gpt(disk))
        return DISKTABLE_GPT;
    else if (check_mbr(disk))
        return DISKTABLE_MBR;
    else
        return DISKTABLE_NONE;
}

void init_disk_manager(void)
{
    disk_list.head = NULL;
    disk_list.tail = NULL;
}

int register_disk(void* control, disk_ops_t* ops)
{
    disk_list_entry_t* entry;
    disk_t disk;

    disk.id = get_next_disk_id();
    disk.ops = ops;
    disk.control = control;
    disk.partition_table_type = get_partition_table_type(&disk);

    entry = malloc(sizeof(disk_list_entry_t));
    if (entry == NULL)
    {
        trace_disk("Could not allocate space for new disk list entry");
        return -1;
    }

    entry->disk = disk;
    entry->next = NULL;

    if (disk_list.tail == NULL)
        disk_list.head = entry;
    else
        disk_list.tail->next = entry;
    disk_list.tail = entry;

    info("Registered new disk with ID %u, it has %s partition table", disk.id, (disk.partition_table_type == DISKTABLE_GPT) ? "GPT" : (disk.partition_table_type == DISKTABLE_MBR) ? "MBR" : "no");

    return 0;
}