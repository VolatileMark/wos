#ifndef __DEVFS_H__
#define __DEVFS_H__

#include "../../../proc/vfs/vfs.h"
#include "../../../proc/vfs/vnode.h"

int devfs_init(vfs_t* vfs);
int devfs_add_device(const char* path, vnode_t* node);

#endif