#include "mbr.h"
#include "../../../../utils/alloc.h"

void mbr_load(drive_t* drive, mbr_t* mbr)
{
    drivefs_read(drive, 0, sizeof(mbr_t), mbr);
}

int mbr_check(drive_t* drive)
{
    mbr_t mbr;
    mbr_load(drive, &mbr);
    return (mbr.signature == MBR_BOOTSECTOR_SIG);
}

int mbr_find_partitions(drive_t* drive, mbr_t* mbr)
{
    uint32_t num_partitions, i;
    mbr_partition_t* partitions[4];
    
    partitions[0] = &mbr->partition1;
    partitions[1] = &mbr->partition2;
    partitions[2] = &mbr->partition3;
    partitions[3] = &mbr->partition4;

    for (num_partitions = 0; num_partitions < 4 && partitions[num_partitions]->type != MBR_PART_NULL; num_partitions++);
    drive->partitions = malloc(sizeof(drive_partition_t) * num_partitions);

    for (i = 0; i < num_partitions; i++)
    {
        drive->partitions[i].lba_start = partitions[i]->lba_start;
        drive->partitions[i].lba_end = partitions[i]->lba_start + partitions[i]->sectors;
        drive->partitions[i].sectors = partitions[i]->sectors;
        drive->partitions[i].flags = partitions[i]->attributes;
    }

    return num_partitions;
}
