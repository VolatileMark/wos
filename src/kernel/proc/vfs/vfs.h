#ifndef __VFS_H__
#define __VFS_H__

#include "vnode.h"
#include "vattribs.h"
#include <stdint.h>

struct vfs_ops;

typedef struct vfs
{
    struct vfs_ops* ops;
    const char* mount_path;
} vfs_t;

typedef struct vfs_ops
{
    int (*root)(vfs_t* vfs, vnode_t* root);
} vfs_ops_t;

void vfs_init(void);

int vfs_mount(const char* path, vfs_t* vfs);
int vfs_lookup(const char* path, vnode_t* out);
int vfs_open(vnode_t* target);
int vfs_read(vnode_t* target, void* buffer, uint64_t bytes);
int vfs_write(vnode_t* target, void* data, uint64_t bytes);
int vfs_get_attribs(vnode_t* target, vattribs_t* out);

#endif