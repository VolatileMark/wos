#ifndef __MBR_H__
#define __MBR_H__

#include "../../fs/drivefs.h"

#define MBR_PART_NULL 0x00
#define MBR_PART_FAT12 0x01
#define MBR_PART_FAT16_MAX32MB 0x04
#define MBR_PART_FAT16 0x06
#define MBR_PART_FAT16_INT13 0x0E
#define MBR_PART_FAT32 0x0B
#define MBR_PART_FAT32_INT13 0x0C
#define MBR_PART_NTFS 0x07
#define MBR_PART_EXT234 0x83
#define MBR_PART_GPT 0xEE
#define MBR_PART_EFI 0xEF

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

void mbr_load(drive_t* drive, mbr_t* mbr);
int mbr_check(drive_t* drive);
int mbr_find_partitions(drive_t* drive, mbr_t* mbr);

#endif