#include "ustar.h"
#include <mem.h>
#include <string.h>

#define USTAR_SIG "ustar"

struct ustar_node
{
    uint8_t file_name[100];
    uint8_t file_mode[8];
    uint8_t owner_id[8];
    uint8_t group_id[8];
    uint8_t file_size[12];
    uint8_t last_modification_time[12];
    uint8_t header_checksum[8];
    uint8_t file_type;
    uint8_t linked_file_name[100];
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

static uint64_t fs_addr;

static uint64_t oct2int(char* str, uint64_t len)
{
    uint64_t num = 0;
    char* c = str;
    while (len-- > 0)
    {
        num *= 8;
        num += *c - '0';
        c++;
    }
    return num;
}

void ustar_set_address(uint64_t addr)
{
    fs_addr = addr;
}

uint64_t ustar_lookup(char* name, uint64_t* out)
{
    ustar_node_t* entry = (ustar_node_t*) fs_addr;
    while(strcmp((char*) entry->ustar_signature, USTAR_SIG) == 0)
    {
        uint64_t file_size = oct2int((char*) entry->file_size, sizeof(entry->file_size) - 1);
        if (strcmp((char*) entry->file_name, name) == 0)
        {
            *out = ((uint64_t) entry) + sizeof(ustar_node_t);
            return file_size;
        }
        entry = (ustar_node_t*) alignu((uint64_t)(entry + 1) + file_size, 512);
    }
    return 0;
}
