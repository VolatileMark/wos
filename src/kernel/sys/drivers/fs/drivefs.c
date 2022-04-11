#include "drivefs.h"
#include "devfs.h"
#include "isofs.h"
#include "../storage/partition-tables/mbr.h"
#include "../storage/partition-tables/gpt.h"
#include "../../../proc/vfs/vnode.h"
#include "../../../utils/alloc.h"
#include "../../../utils/macros.h"
#include "../../../utils/log.h"
#include <stddef.h>

#define trace_drive_manager(msg, ...) trace("STOR", msg, ##__VA_ARGS__)

typedef drive_partition_fs_t (*fs_check_t)(drive_t* drive);

static uint64_t drive_index;
static vnode_ops_t drive_manager_vnode_ops;
static fs_check_t fss[] = {
    &isofs_check
};

static int drivefs_open_stub(vnode_t* node)
{
    UNUSED(node);
    return -1;
}

static int drivefs_read_stub(vnode_t* node, void* buffer, uint64_t count)
{
    UNUSED(node);
    UNUSED(buffer);
    UNUSED(count);
    return -1;
}

static int drivefs_write_stub(vnode_t* node, const char* data, uint64_t count)
{
    UNUSED(node);
    UNUSED(data);
    UNUSED(count);
    return -1;
}

static int drivefs_get_attribs_stub(vnode_t* node, vattribs_t* attr)
{
    UNUSED(node);
    UNUSED(attr);
    return 0;
}

static int drivefs_lookup_stub(vnode_t* dir, const char* path, vnode_t* out)
{
    UNUSED(dir);
    UNUSED(path);
    UNUSED(out);
    return -1;
}

static int drivefs_detect_gpt_partitions(drive_t* drive)
{
    gpt_t gpt;
    gpt_load(drive, &gpt);
    return gpt_find_partitions(drive, &gpt);
}

static int drivefs_detect_mbr_partitions(drive_t* drive)
{
    mbr_t mbr;
    mbr_load(drive, &mbr);
    return mbr_find_partitions(drive, &mbr);
}

static int drivefs_detect_partitions(drive_t* drive)
{
    if (gpt_check(drive)) 
        return drivefs_detect_gpt_partitions(drive);
    else if (mbr_check(drive))
        return drivefs_detect_mbr_partitions(drive);
    return 0;
}

static drive_partition_fs_t drivefs_detect_partition_fs(drive_t* drive, uint64_t index)
{
    drive_partition_fs_t fs;
    uint64_t i;
    for (i = 0; i < (sizeof(fss) / sizeof(uint64_t)) && (fs = fss[i](drive)) == DRIVE_FS_UNKNOWN; i++);
    return fs;
}

int drivefs_register_drive(void* interface, drive_ops_t* ops, uint64_t sector_bytes)
{
    uint64_t i;
    drive_t* drive;
    vnode_t* vnode;
    char path[] = "sda";

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

    if (drivefs_identify(drive))
    {
        free(drive);
        trace_drive_manager("Failed to IDENTIFY drive");
    }
    drive->sector_bytes = drive->ops->property(drive->interface, DRIVE_PROPERTY_SECSIZE);
    
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

    drive->num_partitions = drivefs_detect_partitions(drive);
    info("Added drive %u as /dev/%s. Found %d partition%c", drive_index, path, drive->num_partitions, (drive->num_partitions == 1) ? ' ' : 's');
    if (drive->num_partitions > 0)
    {
        for (i = 0; i < drive->num_partitions; i++)
        {
            drive->partitions[i].fs = drivefs_detect_partition_fs(drive, i);
            info("Detected partition of type %u saved as /dev/%s%u", drive->partitions[i].fs, path, i + 1);
        }
    }

    drive_index++;

    return 0;
}

void drivefs_init(void)
{
    drive_index = 0;
    drive_manager_vnode_ops.open = &drivefs_open_stub;
    drive_manager_vnode_ops.lookup = &drivefs_lookup_stub;
    drive_manager_vnode_ops.read = &drivefs_read_stub;
    drive_manager_vnode_ops.write = &drivefs_write_stub;
    drive_manager_vnode_ops.get_attribs = &drivefs_get_attribs_stub;
}

drive_t* drivefs_lookup(const char* path)
{
    vnode_t out;
    vfs_lookup(path, &out);
    return ((drive_t*) out.data);
}

uint64_t drivefs_read(drive_t* drive, uint64_t lba, uint64_t bytes, void* buffer)
{
    return drive->ops->read(drive->interface, lba, bytes, buffer);
}

int drivefs_identify(drive_t* drive)
{
    return drive->ops->identify(drive->interface);
}

uint64_t drivefs_property(drive_t* drive)
{
    return drive->ops->identify(drive->interface);
}