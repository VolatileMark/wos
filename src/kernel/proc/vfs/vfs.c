#include "vfs.h"
#include "../../utils/log.h"
#include "../../utils/alloc.h"
#include <stddef.h>
#include <string.h>
#include <mem.h>

#define trace_vfs(msg, ...) trace("VIFS", msg, ##__VA_ARGS__) 

typedef struct vfs_list_entry
{
    struct vfs_list_entry* next;
    vfs_t* vfs;
} vfs_list_entry_t;

typedef struct
{
    vfs_list_entry_t* head;
    vfs_list_entry_t* tail;
} vfs_list_t;

static vfs_list_t vfs_list;

void vfs_init(void)
{
    vfs_list.head = NULL;    
    vfs_list.tail = NULL;
}

int vfs_mount(const char* path, vfs_t* vfs)
{
    vfs_list_entry_t* entry;
    uint64_t path_bytes;

    entry = calloc(1, sizeof(vfs_list_entry_t));
    if (entry == NULL)
    {
        trace_vfs("Could not allocate space for new entry");
        return -1;
    }
    entry->vfs = vfs;
    
    if (vfs_list.tail == NULL)
        vfs_list.head = entry;
    else
        vfs_list.tail->next = entry;
    vfs_list.tail = entry;

    path_bytes = strlen(path) + 1;
    vfs->mount_path = malloc(path_bytes);
    memcpy((void*) vfs->mount_path, path, path_bytes);

    return 0;
}

static uint64_t find_longest_mount_path(const char* path, vfs_t** vfs_root)
{
    uint64_t path_len, max_path_len;
    char* path_ptr;
    char* mount_path_ptr;
    vfs_list_entry_t* entry;

    for
    (
        entry = vfs_list.head, max_path_len = 0, *vfs_root = NULL;
        entry != NULL;
        entry = entry->next
    )
    {
        for
        (
            path_len = 0, mount_path_ptr = (char*) entry->vfs->mount_path, path_ptr = (char*) path;
            *path_ptr != '\0' && *mount_path_ptr != '\0' && *path_ptr++ == *mount_path_ptr++;
            path_len++
        );

        if (path_len > max_path_len && *mount_path_ptr == '\0')
        {
            *vfs_root = entry->vfs;
            max_path_len = path_len;
        }
    }

    return max_path_len;
}

int vfs_lookup(const char* path, vnode_t* out)
{
    uint64_t path_length, slash_offset;
    vfs_t* mount_vfs;
    vnode_t node;
    char* new_path;
    char* new_path_ptr;
    int exit_code;

    path_length = find_longest_mount_path(path, &mount_vfs);
    path += path_length;

    if (mount_vfs->ops->root(mount_vfs, &node))
    {
        trace_vfs("Failed to retrieve root node for path: %s", path);
        return -1;
    }
    
    path_length = strlen(path) + 1;
    if (path_length == 0)
    {
        out->ops = node.ops;
        out->data = node.data;
        return 0;
    }
    
    new_path = malloc(path_length);
    new_path_ptr = new_path;
    exit_code = 0;

    memcpy(new_path, path, path_length);

    while (strlen(new_path_ptr) > 0 && exit_code == 0)
    {
        slash_offset = strcspn(new_path_ptr + 1, "/") + 1;
        if (slash_offset != strlen(new_path_ptr))
            new_path_ptr[slash_offset++] = '\0';
        exit_code = node.ops->lookup(&node, new_path_ptr, &node);
        new_path_ptr += slash_offset;
    }

    free(new_path);
    
    out->data = node.data;
    out->ops = node.ops;

    if (exit_code != 0)
        trace_vfs("Lookup error: %d", (long) exit_code);

    return exit_code;
}

int vfs_open(vnode_t* target)
{
    return target->ops->open(target);
}

int vfs_read(vnode_t* target, void* buffer, uint64_t bytes)
{
    return target->ops->read(target, buffer, bytes);
}

int vfs_write(vnode_t* target, void* data, uint64_t bytes)
{
    return target->ops->write(target, data, bytes);
}

int vfs_get_attribs(vnode_t* target, vattribs_t* out)
{
    return target->ops->get_attribs(target, out);
}