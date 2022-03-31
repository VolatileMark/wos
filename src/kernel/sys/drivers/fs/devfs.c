#include "devfs.h"
#include "../../../utils/macros.h"
#include "../../../utils/log.h"
#include "../../../utils/alloc.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define trace_devfs(msg, ...) trace("DVFS", msg, ##__VA_ARGS__)

typedef struct devfs_inode
{
    struct devfs_inode* next;
    const char* name;
    vnode_t* vnode;
} devfs_inode_t;

typedef struct
{
    devfs_inode_t* head;
    devfs_inode_t* tail;
} devfs_inode_list_t;

typedef struct devfs_vfss_list_entry
{
    struct devfs_vfss_list_entry* next;
    vnode_t root;
} devfs_vfss_list_entry_t;

typedef struct
{
    devfs_vfss_list_entry_t* head;
    devfs_vfss_list_entry_t* tail;
} devfs_vfss_list_t;

static uint64_t index;
static devfs_vfss_list_t vfss_list;
static devfs_inode_list_t inodes_list;
static vnode_ops_t vnode_ops;
static vfs_ops_t vfs_ops;

static devfs_vfss_list_entry_t* devfs_get_entry(uint64_t index)
{
    devfs_vfss_list_entry_t* entry;
    for 
    (
        entry = vfss_list.head; 
        index > 0 && entry != NULL; 
        index--, entry = entry->next
    );
    if (index > 0)
        return NULL;
    return entry;
}

static int devfs_root(vfs_t* vfs, vnode_t* out)
{
    devfs_vfss_list_entry_t* entry;

    entry = devfs_get_entry(vfs->index);
    if (entry == NULL)
        return -1;
    out->ops = entry->root.ops;
    out->data = entry->root.data;
    return 0;
}

static int devfs_open(vnode_t* node)
{
    UNUSED(node);
    return -1;
}

static int devfs_read(vnode_t* node, void* buffer, uint64_t count)
{
    UNUSED(node);
    UNUSED(buffer);
    UNUSED(count);
    return -1;
}

static int devfs_write(vnode_t* node, const char* data, uint64_t count)
{
    UNUSED(node);
    UNUSED(data);
    UNUSED(count);
    return -1;
}

static int devfs_get_attribs(vnode_t* node, vattribs_t* attr)
{
    UNUSED(node);
    attr->size = 0;
    return 0;
}

static int devfs_lookup(vnode_t* dir, const char* path, vnode_t* out)
{
    devfs_inode_t* inode;

    UNUSED(dir);

    if (path == NULL)
    {
        trace_devfs("Trying to lookup a NULL path");
        return -1;
    }

    for (inode = inodes_list.head; inode != NULL; inode = inode->next)
    {
        if (strcmp(inode->name, path) == 0)
        {
            out->data = inode->vnode->data;
            out->ops = inode->vnode->ops;
            return 0;
        }
    }

    return -1;
}

int devfs_add_device(const char* path, vnode_t* node)
{
    devfs_inode_t* inode;
    uint64_t path_len;

    if (path == NULL || strlen(path) == 0)
    {
        trace_devfs("Trying to add device with invalid path");
        return -1;
    }

    for (inode = inodes_list.head; inode != NULL; inode = inode->next)
    {
        if (strcmp(inode->name, path) == 0)
        {
            trace_devfs("Trying to add device (%s) that already exists", inode->name);
            return -1;
        }
    }

    path_len = strlen(path) + 1;
    inode = malloc(sizeof(devfs_inode_t));
    if (inode == NULL)
    {
        trace_devfs("Could not allocate space for new inode (%s)", path);
        return -1;
    }

    inode->name = malloc(path_len);
    if (inode->name == NULL)
    {
        trace_devfs("Could not allocate space for new inode's path (%s)", path);
        free(inode);
        return -1;
    }

    strcpy((char*) inode->name, path);
    inode->next = NULL;
    inode->vnode = node;

    if (inodes_list.tail == NULL)
        inodes_list.head = inode;
    else
        inodes_list.tail->next = inode;
    inodes_list.tail = inode;

    return 0;
}


int devfs_create(vfs_t* vfs)
{
    devfs_vfss_list_entry_t* entry;

    entry = malloc(sizeof(devfs_vfss_list_entry_t));
    if (entry == NULL)
        return -1;

    entry->root.ops = &vnode_ops;
    entry->next = NULL;
    if (vfss_list.tail == NULL)
        vfss_list.head = entry;
    else
        vfss_list.tail->next = entry;
    vfss_list.tail = entry;

    vfs->index = index++;
    vfs->ops = &vfs_ops;
    return 0;
}

void devfs_init(void)
{
    index = 0;
    vfss_list.head = NULL;
    vfss_list.tail = NULL;
    vnode_ops.open = &devfs_open;
    vnode_ops.read = &devfs_read;
    vnode_ops.write = &devfs_write;
    vnode_ops.lookup = &devfs_lookup;
    vnode_ops.get_attribs = &devfs_get_attribs;
    vfs_ops.root = &devfs_root;
}