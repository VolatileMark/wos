#ifndef __DRIVEFS_H__
#define __DRIVEFS_H__

#include "../../../proc/vfs/vfs.h"

struct drive_ops;

typedef enum
{
    DRIVE_PROPERTY_SECSIZE
} drive_property_t;

typedef struct
{
    char fs_sig[8];
    vfs_t vfs;
    uint64_t lba_start;
    uint64_t lba_end;
    uint64_t sectors;
    uint64_t flags;
} drive_partition_t;

typedef struct
{
    void* interface;
    struct drive_ops* ops;
    uint32_t num_partitions;
    uint32_t sector_bytes;
    drive_partition_t* partitions;
} drive_t;

typedef struct drive_ops
{
    uint64_t (*read)(drive_t* drive, uint64_t lba, uint64_t size, void* buffer);
} drive_ops_t;

int drivefs_register_drive(drive_t* drive);
void drivefs_init(void);

drive_t* drivefs_lookup(const char* path);
uint64_t drivefs_read(drive_t* drive, uint64_t lba, uint64_t bytes, void* buffer);

#endif
