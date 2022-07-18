#include "vnode.h"
#include <mem.h>

void vnode_copy(vnode_t* src, vnode_t* dest)
{
    memcpy(dest, src, sizeof(vnode_t));
}
