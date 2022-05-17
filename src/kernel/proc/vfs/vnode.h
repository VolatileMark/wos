#ifndef __VNODE_H__
#define __VNODE_H__

#include "vattribs.h"

struct vnode_ops;

typedef struct
{
    struct vnode_ops* ops;
    void* data;
} vnode_t;

typedef struct vnode_ops
{
    int (*open)(vnode_t* this_node);
    int (*lookup)(vnode_t* this_node, const char* path, vnode_t* out);
    int (*read)(vnode_t* this_node, void* buffer, uint64_t bytes);
    int (*write)(vnode_t* this_node, const char* data, uint64_t bytes);
    int (*get_attribs)(vnode_t* this_node, vattribs_t* out);
} vnode_ops_t;

void vnode_copy(vnode_t* src, vnode_t* dest);

#endif