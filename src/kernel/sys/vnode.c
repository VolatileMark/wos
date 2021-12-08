#include "vnode.h"

void vnode_copy(vnode_t* src, vnode_t* dest)
{
    dest->op = src->op;
    dest->data = src->data;
}