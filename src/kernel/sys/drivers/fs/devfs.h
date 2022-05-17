#ifndef __DEVFS_H__
#define __DEVFS_H__

#include "../../../proc/vfs/vfs.h"

void devfs_init(void);
int devfs_create(vfs_t* vfs);
int devfs_add_device(const char* path, vnode_t* node);

#endif