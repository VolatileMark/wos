#include "vnode.h"
#include <mem.h>

void copy_vnode(vnode_t* src, vnode_t* dest)
{
    memcpy(dest, src, sizeof(vnode_t));
}