#ifndef __GPT_H__
#define __GPT_H__

#include "../../fs/drivefs.h"
#include "../../../../utils/guid.h"
#include "mbr.h"
#include <stdint.h>

#define GPT_GUID(da1, da2, da3, da4, da5) ((guid_t) { .d1 = da1, .d2 = da2, .d3 = da3, .d4 = da4, .d5 = da5 })

#define GPT_ENTRY_NULL GPT_GUID(0, 0, 0, 0, 0)

#define GPT_SIG "EFI PART"
#define GPT_BLOCK_SIZE 512

struct gpt
{
    char signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t rsv0;
    uint64_t this_header_lba;
    uint64_t backup_header_lba;
    uint64_t first_usable_block;
    uint64_t last_usable_block;
    guid_t disk_guid;
    uint64_t gpea_lba;
    uint32_t gpea_num_entries;
    uint32_t gpea_entry_size;
    uint32_t gpea_crc32;
    uint8_t rsv1[GPT_BLOCK_SIZE - 0x5C];
} __attribute__((packed));
typedef struct gpt gpt_t;

struct gpt_entry
{
    guid_t type_guid;
    guid_t partition_guid;
    uint64_t start_lba;
    uint64_t end_lba;
    uint64_t attributes;
    char partition_name[];
} __attribute__((packed));
typedef struct gpt_entry gpt_entry_t;

void gpt_load(drive_t* drive, gpt_t* gpt);
int gpt_check(drive_t* drive);
int gpt_find_partitions(drive_t* drive, gpt_t* gpt);

#endif