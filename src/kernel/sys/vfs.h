#ifndef __VFS_H__
#define __VFS_H__ 1

#include <stdint.h>
#include "vnode.h"
#include "vattr.h"

struct vfs_ops;

typedef struct vfs
{
    struct vfs* next;
    struct vfs_ops* op;
    const char* mount_path;
} vfs_t;

typedef struct vfs_ops
{
    int (*root)(vfs_t* vfs, vnode_t* root);
} vfs_ops_t;

void init_vfs(void);

int vfs_mount(const char* path, vfs_t* vfs);
int vfs_lookup(const char* path, vnode_t* out);
int vfs_open(vnode_t* node);
int vfs_read(vnode_t* node, void* buffer, uint64_t count);
int vfs_write(vnode_t* node, const uint8_t* buffer, uint64_t count);
int vfs_get_attribs(vnode_t* node, vattribs_t* attr);

#endif