#include "mbr.h"

int check_mbr(disk_t* disk)
{
    mbr_t mbr;
    disk->ops->read(disk, 0, MBR_BLOCK_SIZE, &mbr);
    return (mbr.signature == MBR_BOOTSECTOR_SIG);
}