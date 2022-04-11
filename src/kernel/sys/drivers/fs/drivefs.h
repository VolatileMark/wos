#ifndef __DRIVEFS_H__
#define __DRIVEFS_H__

#include <stdint.h>

struct drive_ops;

typedef enum
{
    DRIVE_FS_UNKNOWN,
    DRIVE_FS_FAT,
    DRIVE_FS_ISO9660
} drive_partition_fs_t;

typedef enum
{
    DRIVE_PROPERTY_SECSIZE
} drive_property_t;

typedef struct
{
    drive_partition_fs_t fs;
    uint64_t lba_start;
    uint64_t lba_end;
    uint64_t sectors;
    uint64_t flags;
    const char* mountpoint;
} drive_partition_t;

typedef struct
{
    void* interface;
    struct drive_ops* ops;
    uint64_t sector_bytes;
    uint64_t num_partitions;
    drive_partition_t* partitions;
} drive_t;

typedef struct drive_ops
{
    int (*identify)(void* inteface);
    uint64_t (*property)(void* interface, drive_property_t property);
    uint64_t (*read)(void* interface, uint64_t lba, uint64_t size, void* buffer);
} drive_ops_t;

int drivefs_register_drive(void* interface, drive_ops_t* ops, uint64_t sector_bytes);
void drivefs_init(void);

drive_t* drivefs_lookup(const char* path);
uint64_t drivefs_read(drive_t* drive, uint64_t lba, uint64_t bytes, void* buffer);
int drivefs_identify(drive_t* drive);
uint64_t drivefs_property(drive_t* drive);

#endif