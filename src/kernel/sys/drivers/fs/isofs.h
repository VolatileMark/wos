#ifndef __ISOFS_H__
#define __ISOFS_H__

#include "drivefs.h"
#include "../../../proc/vfs/vfs.h"

void isofs_init(void);
int isofs_create(drive_t* drive, uint64_t partition_index);

#endif