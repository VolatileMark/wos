#ifndef __VNODE_H__
#define __VNODE_H__ 1

#include <stdint.h>
#include "vattr.h"

struct vnode_ops;

typedef struct
{
    struct vnode_ops* op;
    void* data;
} vnode_t;

typedef struct vnode_ops
{
    int (*open)(vnode_t* node);
    int (*lookup)(vnode_t* dir, const char* name, vnode_t* out);
    int (*read)(vnode_t* node, void* buffer, uint64_t count);
    int (*write)(vnode_t* node, const uint8_t* buffer, uint64_t count);
    int (*get_attribs)(vnode_t* node, vattribs_t* attr);
} vnode_ops_t;

void vnode_copy(vnode_t* src, vnode_t* dest);

#endif