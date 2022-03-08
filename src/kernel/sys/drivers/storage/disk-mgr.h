#ifndef __DRIVE_MGR_H__
#define __DRIVE_MGR_H__

#include <stdint.h>

struct disk;

typedef enum
{
    DISKTABLE_MBR,
    DISKTABLE_GPT,
    DISKTABLE_NONE
} partition_table_type_t;

typedef struct
{
    uint64_t (*read)(struct disk* drive, uint64_t lba, uint64_t bytes, void* buffer);
} disk_ops_t;

typedef struct disk
{
    uint64_t id;
    void* control;
    disk_ops_t* ops;
    partition_table_type_t partition_table_type;
    uint64_t partition_table_lba;
} disk_t;

void init_disk_manager(void);
int register_disk(void* control, disk_ops_t* ops);

#endif