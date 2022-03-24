#include "drivefs.h"
#include "../fs/devfs.h"
#include "../../../proc/vfs/vnode.h"
#include "../../../utils/alloc.h"
#include "../../../utils/macros.h"
#include "../../../utils/log.h"
#include <stddef.h>

#define trace_drive_manager(msg, ...) trace("STOR", msg, ##__VA_ARGS__)

static uint64_t drive_index;
static vnode_ops_t drive_manager_vnode_ops;;

static int drivefs_dummy_open(vnode_t* node)
{
    UNUSED(node);
    return -1;
}

static int drivefs_dummy_read(vnode_t* node, void* buffer, uint64_t count)
{
    UNUSED(node);
    UNUSED(buffer);
    UNUSED(count);
    return -1;
}

static int drivefs_dummy_write(vnode_t* node, const char* data, uint64_t count)
{
    UNUSED(node);
    UNUSED(data);
    UNUSED(count);
    return -1;
}

static int drivefs_dummy_get_attribs(vnode_t* node, vattribs_t* attr)
{
    UNUSED(node);
    UNUSED(attr);
    return 0;
}

static int drivefs_dummy_lookup(vnode_t* dir, const char* path, vnode_t* out)
{
    UNUSED(dir);
    UNUSED(path);
    UNUSED(out);
    return -1;
}

int drivefs_register_drive(void* interface, drive_ops_t* ops)
{
    drive_t* drive;
    vnode_t* vnode;
    char path[4] = "sda";

    if (drive_index > 'z' - 'a')
    {
        trace_drive_manager("Maximum drive number reached");
        return -1;
    }

    drive = malloc(sizeof(drive_t));
    if (drive == NULL)
    {
        trace_drive_manager("Failed to allocate memory for drive descriptor");
        return -1;
    }
    drive->interface = interface;
    drive->ops = ops;

    vnode = malloc(sizeof(vnode_t));
    if (vnode == NULL)
    {
        free(drive);
        trace_drive_manager("Failed to allocate memory for new devfs vnode");
        return -1;
    }

    vnode->data = drive;
    vnode->ops = &drive_manager_vnode_ops;

    path[2] += drive_index;

    if (devfs_add_device((const char*) path, vnode))
    {
        free(drive);
        free(vnode);
        return -1;
    }

    info("Added drive %u as /dev/%s", drive_index++, path);

    return 0;
}

void drivefs_init(void)
{
    drive_index = 0;
    drive_manager_vnode_ops.open = &drivefs_dummy_open;
    drive_manager_vnode_ops.lookup = &drivefs_dummy_lookup;
    drive_manager_vnode_ops.read = &drivefs_dummy_read;
    drive_manager_vnode_ops.write = &drivefs_dummy_write;
    drive_manager_vnode_ops.get_attribs = &drivefs_dummy_get_attribs;
}

uint64_t drivefs_read(drive_t* drive, uint64_t lba, uint64_t bytes, void* buffer)
{
    return drive->ops->read(drive->interface, lba, bytes, buffer);
}