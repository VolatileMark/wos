#ifndef __DRIVEFS_H__
#define __DRIVEFS_H__

#include <stdint.h>

struct drive_ops;

typedef struct
{
    void* interface;
    struct drive_ops* ops;
} drive_t;

typedef struct drive_ops
{
    uint64_t (*read)(void* interface, uint64_t lba, uint64_t size, void* buffer);
} drive_ops_t;

int drivefs_register_drive(void* interface, drive_ops_t* ops);
void drivefs_init(void);

drive_t* drivefs_get_drive(const char* path);
uint64_t drivefs_read(drive_t* drive, uint64_t lba, uint64_t bytes, void* buffer);

#endif