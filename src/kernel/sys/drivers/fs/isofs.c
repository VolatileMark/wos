#include "isofs.h"
#include "../../../proc/vfs/vnode.h"
#include "../../../proc/vfs/vattribs.h"
#include "../../../utils/macros.h"
#include "../../../utils/alloc.h"
#include <stddef.h>
#include <string.h>
#include <mem.h>

#define ISOFS_SIG "ISO9660"
#define ISOFS_MAX_VDS 3
#define ISOFS_SECTOR_SIZE 2048
#define ISOFS_VOLDESC_START 16
#define ISOFS_VOLDESC_SIG "CD001"
#define ISOFS_VOLDESC_VER 0x01
#define ISOFS_VOLDESC_BOOT 0
#define ISOFS_VOLDESC_PRIM 1
#define ISOFS_VOLDESC_TERM 255

#define isofs_block_to_lba(block, sector) (((block) * ISOFS_SECTOR_SIZE) / (sector))

struct isofs_date
{
    char year[4];
    char month[2];
    char day[2];
    char hour[2];
    char minute[2];
    char second[2];
    char cents[2];
    uint8_t gmt_offset; /* Offset from GMT in 15 minutes intervals from -48 east to +52 west */
} __attribute__((packed));
typedef struct isofs_date isofs_date_t;

struct isofs_directory_entry_date
{
    uint8_t years_since_1900; /* # of years since 1900 */
    uint8_t month_of_year; /* Month of year (1-12) */
    uint8_t day_of_month; /* Day of month (1-31) */
    uint8_t hour; /* Hour of the day (0-23) */
    uint8_t minute; /* Minute of the hour (0-59) */
    uint8_t seconds; /* Second of the minute (0-59) */
    uint8_t gmt_offset; /* Offset from GMT in 15 minutes intervals from -48 east to +52 west */
} __attribute__((packed));
typedef struct isofs_directory_entry_date isofs_directory_entry_date_t;

struct isofs_file_flags
{
    uint8_t hidden : 1;
    uint8_t is_directory : 1;
    uint8_t is_associated_file : 1;
    uint8_t extended_has_format_info : 1;
    uint8_t extended_has_group_and_owner : 1;
    uint8_t reserved : 2;
    uint8_t not_final_record : 1;
} __attribute__((packed));
typedef struct isofs_file_flags isofs_file_flags_t;

struct isofs_directory_entry
{
    uint8_t entry_length;
    uint8_t extended_attribute_record_length;
    uint32_t block_lsb;
    uint32_t block_msb;
    uint32_t bytes_lsb;
    uint32_t bytes_msb;
    isofs_directory_entry_date_t date;
    isofs_file_flags_t file_flags;
    uint8_t interleaved_file_unit_size;
    uint8_t interleaved_gap_size;
    uint16_t volume_sequence_number_lsb;
    uint16_t volume_sequence_number_msb;
    uint8_t file_name_length;
    uint8_t file_name[1];
} __attribute__((packed));
typedef struct isofs_directory_entry isofs_directory_entry_t;

struct isofs_volume_descriptor_header
{
    uint8_t type;
    char sig[5];
    uint8_t ver;
} __attribute__((packed));
typedef struct isofs_volume_descriptor_header isofs_volume_descriptor_header_t;

struct isofs_volume_descriptor
{
    isofs_volume_descriptor_header_t hdr;
    uint8_t data[ISOFS_SECTOR_SIZE - sizeof(isofs_volume_descriptor_header_t)];
} __attribute__((packed));
typedef struct isofs_volume_descriptor isofs_volume_descriptor_t;

struct isofs_boot_record
{
    isofs_volume_descriptor_header_t hdr;
    char boot_system_identifier[32];
    char boot_identifier[32];
    uint8_t reserved[ISOFS_SECTOR_SIZE - (sizeof(isofs_volume_descriptor_header_t) + 32*2)];
} __attribute__((packed));
typedef struct isofs_boot_record isofs_boot_record_t;

struct isofs_primary_volume_descriptor
{
    isofs_volume_descriptor_header_t hdr;
    uint8_t zero0;
    char system_identifier[32];
    char vendor_identifier[32];
    uint64_t zero1;
    uint32_t volume_space_size_lsb;
    uint32_t volume_space_size_msb;
    uint8_t zero2[32];
    uint16_t volume_set_size_lsb;
    uint16_t volume_set_size_msb;
    uint16_t volume_sequence_number_lsb;
    uint16_t volume_sequence_number_msb;
    uint16_t logical_block_size_lsb;
    uint16_t logical_block_size_msb;
    uint32_t path_table_size_lsb;
    uint32_t path_table_size_msb;
    uint32_t path_table_lba_lsb;
    uint32_t optional_path_table_lba_lsb;
    uint32_t path_table_lba_msb;
    uint32_t optional_path_table_lba_msb;
    isofs_directory_entry_t root;
    char volume_set_identifier[128];
    char publisher_identifier[128];
    char data_preparer_identifier[128];
    char application_identifier[128];
    char copyright_file_identifier[37];
    char abstract_file_identifier[37];
    char bibliographic_file_identifier[37];
    isofs_date_t volume_creation_date;
    isofs_date_t volume_modification_date;
    isofs_date_t volume_expiration_date;
    isofs_date_t volume_effective_date;
    uint8_t file_structure_version;
    uint8_t zero3;
    uint8_t application_used[512];
    uint8_t reserved[ISOFS_SECTOR_SIZE - 1395];
} __attribute__((packed));
typedef struct isofs_primary_volume_descriptor isofs_primary_volume_descriptor_t;

typedef struct isofs_vfss_list_entry
{
    struct isofs_vfss_list_entry* next;
    vnode_t root;
} isofs_vfss_list_entry_t;

typedef struct
{
    drive_t* drive;
    uint8_t is_directory : 1;
    uint8_t is_hidden : 1;
    uint8_t exists : 1;
    uint8_t reserved : 5;
    uint64_t data_block;
    uint64_t data_size;
} isofs_inode_t;

typedef struct
{
    isofs_vfss_list_entry_t* head;
    isofs_vfss_list_entry_t* tail;
} isofs_vfss_list_t;

static uint64_t index;
static isofs_vfss_list_t vfss_list;
static vfs_ops_t vfs_ops;
static vnode_ops_t vnode_ops;

int isofs_load_volume_descriptor(drive_t* drive, isofs_volume_descriptor_t* desc, uint8_t type)
{
    uint8_t index;

    index = 0;
    do {
        drivefs_read(drive, isofs_block_to_lba(ISOFS_VOLDESC_START + (index++), drive->sector_bytes), sizeof(isofs_volume_descriptor_t), desc);
    } while (desc->hdr.type != type && index < ISOFS_MAX_VDS);

    return -(index == ISOFS_MAX_VDS);
}

int isofs_check(drive_t* drive)
{
    isofs_volume_descriptor_t desc;
    isofs_load_volume_descriptor(drive, &desc, ISOFS_VOLDESC_BOOT);
    if 
    (
        strncmp(desc.hdr.sig, ISOFS_VOLDESC_SIG, 5) == 0 && 
        desc.hdr.ver == ISOFS_VOLDESC_VER
    )
        return 0;
    return -1;
}

static isofs_vfss_list_entry_t* isofs_get_entry(uint64_t index)
{
    isofs_vfss_list_entry_t* entry;
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

static int isofs_root(vfs_t* vfs, vnode_t* out)
{
    isofs_vfss_list_entry_t* entry;
    entry = isofs_get_entry(vfs->index);
    if (entry == NULL)
        return -1;
    out->ops = entry->root.ops;
    out->data = entry->root.data;
    return 0;
}

static int isofs_open(vnode_t* node)
{
    isofs_inode_t* inode;
    inode = node->data;
    if (inode->is_directory || !inode->exists)
        return -1;
    return 0;
}

static int isofs_read(vnode_t* node, void* buffer, uint64_t count)
{
    isofs_inode_t* inode;
    inode = node->data;
    if (inode->is_directory || inode->exists)
        return -1;
    if (drivefs_read(inode->drive, isofs_block_to_lba(inode->data_block, inode->drive->sector_bytes), count, buffer) < count)
        return -1;
    return 0;
}

static int isofs_write(vnode_t* node, const char* data, uint64_t count)
{
    UNUSED(node);
    UNUSED(data);
    UNUSED(count);
    return -1;
}

static int isofs_get_attribs(vnode_t* node, vattribs_t* attr)
{
    isofs_inode_t* inode;
    inode = node->data;
    attr->size = inode->data_size;
    return 0;
}

static int isofs_lookup(vnode_t* dir, const char* path, vnode_t* out)
{
    isofs_directory_entry_t* iso_dir;
    isofs_inode_t* inode;
    uint8_t* sector;
    uint64_t path_offset, bytes_offset;

    inode = dir->data;
    if (!inode->is_directory)
        return -1;

    if (*path == '/')
        ++path;
    for (path_offset = 0; path[path_offset] != '\0' && path[path_offset] != '/'; path_offset++);

    sector = malloc(ISOFS_SECTOR_SIZE);
    if (sector == NULL)
        return -1;
    
    if (drivefs_read(inode->drive, isofs_block_to_lba(inode->data_block, inode->drive->sector_bytes), inode->data_size, sector) < ISOFS_SECTOR_SIZE)
    {
        free(sector);
        return -1;
    }

    for 
    (
        bytes_offset = 0, iso_dir = (isofs_directory_entry_t*) sector; 
        bytes_offset < inode->data_size && bytes_offset < ISOFS_SECTOR_SIZE && iso_dir->entry_length != 0;
        bytes_offset += iso_dir->entry_length
    ) 
    {
        iso_dir = (isofs_directory_entry_t*) (sector + bytes_offset);
        if 
        (
            iso_dir->file_name_length >= path_offset &&
            strncmp(path, (const char*) iso_dir->file_name, path_offset) == 0
        )
            goto SUCCESS;
    }

    free(sector);
    return -1;

SUCCESS:
    out->ops = &vnode_ops;
    out->data = malloc(sizeof(isofs_inode_t));
    ((isofs_inode_t*) out->data)->drive = inode->drive;
    inode = out->data;
    inode->is_directory = iso_dir->file_flags.is_directory;
    inode->is_hidden = iso_dir->file_flags.hidden;
    inode->exists = 1;
    inode->data_block = iso_dir->block_lsb;
    inode->data_size = iso_dir->bytes_lsb;

    free(sector);
    return 0;
}

int isofs_create(drive_t* drive, uint64_t partition_index)
{
    isofs_inode_t* inode;
    isofs_vfss_list_entry_t* entry;
    isofs_primary_volume_descriptor_t pvd;
    
    if 
    (
        isofs_check(drive) ||
        isofs_load_volume_descriptor(drive, (isofs_volume_descriptor_t*) &pvd, ISOFS_VOLDESC_PRIM)
    )
        return -1;

    entry = malloc(sizeof(isofs_vfss_list_entry_t));
    if (entry == NULL)
        return -1;
    
    entry->root.ops = &vnode_ops;
    inode = malloc(sizeof(isofs_inode_t));
    if (inode == NULL)
    {
        free(entry);
        return -1;
    }
    inode->drive = drive;
    inode->is_hidden = pvd.root.file_flags.hidden;
    inode->is_directory = pvd.root.file_flags.is_directory;
    inode->data_block = pvd.root.block_lsb;
    inode->data_size = pvd.root.bytes_lsb;
    entry->root.data = inode;

    entry->next = NULL;
    if (vfss_list.tail == NULL)
        vfss_list.head = entry;
    else
        vfss_list.tail->next = entry;
    vfss_list.tail = entry;

    drive->partitions[partition_index].vfs.index = index++;
    drive->partitions[partition_index].vfs.ops = &vfs_ops;
    strcpy(drive->partitions[partition_index].fs_sig, ISOFS_SIG);

    return 0;
}

void isofs_init(void)
{
    index = 0;
    vfss_list.head = NULL;
    vfss_list.tail = NULL;
    vnode_ops.open = &isofs_open;
    vnode_ops.read = &isofs_read;
    vnode_ops.write = &isofs_write;
    vnode_ops.lookup = &isofs_lookup;
    vnode_ops.get_attribs = &isofs_get_attribs;
    vfs_ops.root = &isofs_root;
}