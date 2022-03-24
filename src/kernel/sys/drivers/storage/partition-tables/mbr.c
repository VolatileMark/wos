#include "mbr.h"

int mbr_check(void* disk)
{
    /*
    mbr_t mbr;
    disk->ops->read(disk, 0, MBR_BLOCK_SIZE, &mbr);
    return (mbr.signature == MBR_BOOTSECTOR_SIG);
    */
    return 0;
}