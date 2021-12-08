#include "vfs.h"
#include "../utils/alloc.h"
#include <stddef.h>
#include <string.h>
#include <mem.h>

static vfs_t* vfs_list;

static int vfs_find_longest_mount_path(const char* path, vfs_t** out)
{
    vfs_t* i = vfs_list;
    vfs_t* root;
    uint64_t max_length = 0;
    uint64_t current_length;
    const char* mount_path_ptr;
    const char* path_ptr;

    while (i != NULL)
    {
        for 
        (
            current_length = 0, mount_path_ptr = i->mount_path, path_ptr = path; 
            *path_ptr && *mount_path_ptr && *path_ptr == *mount_path_ptr;
            path_ptr ++, mount_path_ptr ++, current_length ++
        );
        if (current_length > max_length)
        {
            root = i;
            max_length = current_length;
        }
        i = i->next;
    }

    *out = root;
    return max_length;
}

void init_vfs(void)
{
    vfs_list = NULL;
}

int vfs_mount(const char* path, vfs_t* vfs)
{
    if (vfs_list == NULL)
        vfs_list = vfs;
    else
    {
        vfs_t* i = vfs_list;
        while (i->next != NULL)
        {
            if (strcmp(i->mount_path, path) == 0)
                return 1;
            i = i->next;
        }
        i->next = vfs;
    }

    int path_length = strlen(path) + 1;
    char* path_buffer = malloc(path_length * sizeof(char));
    memcpy(path_buffer, path, path_length);
    vfs->mount_path = path_buffer;
    vfs->next = NULL;

    return 0;
}

int vfs_lookup(const char* path, vnode_t* out)
{
    if (strlen(path) < 1)
        return 1;
    if (path[0] != '/')
        return 2;
    
    vfs_t* root;
    int max_length = vfs_find_longest_mount_path(path, &root);
    path += max_length;

    vnode_t node;
    if (root->op->root(root, &node))
        return 3;
    
    if (strlen(path) == 0)
    {
        out->op = node.op;
        out->data = node.data;
        return 0;
    }

    int ret = 0;
    int inc;
    uint64_t slash_index;
    uint64_t path_length = strlen(path) + 1;
    char* path_buffer = malloc(path_length * sizeof(char));
    char* path_ptr = path_buffer;
    memcpy(path_buffer, path, path_length);

    while (strlen(path_ptr) != 0 && ret == 0)
    {
        slash_index = strcspn(path_ptr + 1, "/") + 1;

        if (slash_index != strlen(path_ptr))
        {
            path_ptr[slash_index] = '\0';
            inc = slash_index + 1;
        }
        else
            inc = slash_index;
        
        ret = node.op->lookup(&node, path_ptr, out);
        node.op = out->op;
        node.data = out->data;

        path_ptr += inc;
    }

    free(path_buffer);

    return ret;
}

int vfs_open(vnode_t* node)
{
    return node->op->open(node);
}

int vfs_read(vnode_t* node, void* buffer, uint64_t count)
{
    return node->op->read(node, buffer, count);
}

int vfs_write(vnode_t* node, const uint8_t* buffer, uint64_t count)
{
    return node->op->write(node, buffer, count);
}

int vfs_get_attribs(vnode_t* node, vattribs_t* attr)
{
    return node->op->get_attribs(node, attr);
}