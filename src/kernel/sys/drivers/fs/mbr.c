#include "mbr.h"

uint32_t check_mbr(disk_t* disk)
{
    mbr_t mbr;
    disk->ops->read(disk, 0, sizeof(mbr_t), &mbr);
    if (mbr.signature == MBR_BOOTSECTOR_SIG)
    {
        return 
        (
            (mbr.partition1.lba_start != 0) +
            (mbr.partition2.lba_start != 0) + 
            (mbr.partition3.lba_start != 0) +
            (mbr.partition4.lba_start != 0)
        );
    }
    return 0;
}