#include "ustar.h"
#include "../../mem/paging.h"
#include "../../sys/vnode.h"
#include "../../utils/macros.h"
#include <string.h>
#include <stddef.h>
#include <mem.h>

#define USTAR_SIG "ustar"
#define IS_DIRECTORY(entry) ((entry->file_type - '0') == FILE_DIRECTORY)
#define IS_FILE(entry) ((entry->file_type - '0') == FILE_NORMAL)

struct ustar_node
{
    uint8_t file_path[100];
    uint8_t file_mode[8];
    uint8_t owner_id[8];
    uint8_t group_id[8];
    uint8_t file_size[12];
    uint8_t last_modification_time[12];
    uint8_t header_checksum[8];
    uint8_t file_type;
    uint8_t linked_file_path[100];
    uint8_t ustar_signature[6];
    uint8_t ustar_version[2];
    uint8_t owner_name[32];
    uint8_t group_name[32];
    uint8_t device_number_major[8];
    uint8_t device_number_minor[8];
    uint8_t file_name_prefix[155];
    uint8_t reserved[12];
} __attribute__((packed));
typedef struct ustar_node ustar_node_t;

typedef enum
{
    FILE_NORMAL,
    FILE_HARD_LINK,
    FILE_SYMBOLIC_LINK,
    FILE_CHARACTER_DEVICE,
    FILE_BLOCK_DEVICE,
    FILE_DIRECTORY,
    FILE_NAMED_PIPE
} FILE_TYPE;

static vnode_ops_t ustar_vnode_ops;
static vfs_ops_t ustar_vfs_ops;
static ustar_node_t* root_node;

static uint64_t oct2int(char* str, uint64_t len)
{
    uint64_t num = 0;
    uint8_t* c = (uint8_t*) str;
    while (len -- > 0)
    {
        num *= 8;
        num += *c - '0';
        c ++;
    }
    return num;
}

static int ustar_root(vfs_t* vfs, vnode_t* root)
{
    UNUSED(vfs);
    root->op = &ustar_vnode_ops;
    root->data = root_node;
    return 0;
}

static int ustar_open(vnode_t* node)
{
    UNUSED(node);
    return 0;
}

static int ustar_get_attribs(vnode_t* node, vattribs_t* attr)
{
    ustar_node_t* entry = (ustar_node_t*) node->data;
    attr->file_size = oct2int((char*) entry->file_size, 12);
    return 0;
}

static int ustar_lookup(vnode_t* dir, const char* name, vnode_t* out)
{
    ustar_node_t* entry = (ustar_node_t*) dir->data;
    if (!IS_DIRECTORY(entry))
        return 1;

    while (strcmp((char*) entry->ustar_signature, USTAR_SIG) == 0)
    {
        if (strcmp(((char*) entry->file_path) + 1, name) == 0)
        {
            out->op = &ustar_vnode_ops;
            out->data = entry;
            return 0;
        }
        uint64_t file_size = oct2int((char*) entry->file_size, 12);
        entry = (ustar_node_t*) alignu((uint64_t)(entry + 1) + file_size, 512);
    }

    return 2;
}

static int ustar_read(vnode_t* node, void* buffer, uint64_t count)
{
    ustar_node_t* entry = (ustar_node_t*) node->data;
    
    if (buffer == NULL)
        return 1;
    
    if (!IS_FILE(entry))
        return 2;
    
    uint64_t file_size = oct2int((char*) entry->file_size, 12);
    
    if (count > file_size)
        count = file_size;
    
    memcpy(buffer, (uint8_t*) alignu((uint64_t)(entry + 1), 512), count);
    
    return 0;
}

static int ustar_write(vnode_t* node, const uint8_t* buffer, uint64_t count)
{
    UNUSED(node);
    UNUSED(buffer);
    UNUSED(count);
    return 1;
}

int init_ustar(uint64_t addr, uint64_t size, vfs_t* vfs)
{
    paging_map_memory(addr, addr, size, ACCESS_RW, PL0);
    
    root_node = (ustar_node_t*) addr;
    if (strcmp((char*) root_node->ustar_signature, USTAR_SIG) != 0)
        return 1;

    ustar_vnode_ops.open = &ustar_open;
    ustar_vnode_ops.lookup = &ustar_lookup;
    ustar_vnode_ops.read = &ustar_read;
    ustar_vnode_ops.write = &ustar_write;
    ustar_vnode_ops.get_attribs = &ustar_get_attribs;

    ustar_vfs_ops.root = &ustar_root;

    vfs->next = NULL;
    vfs->op = &ustar_vfs_ops;

    return 0;
}