#ifndef __GPT_H__
#define __GPT_H__

#include "../../../../utils/guid.h"
#include "mbr.h"
#include <stdint.h>

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
    char patition_name[72];
} __attribute__((packed));
typedef struct gpt_entry gpt_entry_t;

int gpt_check(void* disk);

#endif