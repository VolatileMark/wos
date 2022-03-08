#ifndef __MBR_H__
#define __MBR_H__

#include "../storage/disk-mgr.h"
#include <stdint.h>

#define MBR_BOOTSECTOR_SIG 0xAA55

struct mbr_partition
{
    uint32_t attributes : 8;
    uint32_t chs_start : 24;
    uint32_t type : 8;
    uint32_t chs_end : 24;
    uint32_t lba_start;
    uint32_t sectors;
} __attribute__((packed));
typedef struct mbr_partition mbr_partition_t;

struct mbr
{
    uint8_t bootstrap[440];
    uint32_t disk_id;
    uint16_t reserved;
    mbr_partition_t partition1;
    mbr_partition_t partition2;
    mbr_partition_t partition3;
    mbr_partition_t partition4;
    uint16_t signature;
} __attribute__((packed));
typedef struct mbr mbr_t;

uint32_t check_mbr(disk_t* disk);

#endif