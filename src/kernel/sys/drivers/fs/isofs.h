#ifndef __ISOFS_H__
#define __ISOFS_H__

#include "drivefs.h"
#include "../../../proc/vfs/vfs.h"

void isofs_init(void);
drive_partition_fs_t isofs_check(drive_t* drive);
int isofs_create(vfs_t* vfs, drive_t* drive, uint64_t partition_index);

#endif