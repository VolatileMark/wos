#include "gpt.h"
#include "mbr.h"
#include "../../../utils/helpers/crc32.h"
#include "../../../utils/helpers/alloc.h"
#include <string.h>

int check_gpt(disk_t* disk)
{
    uint32_t header_crc32;
    gpt_t gpt;
    mbr_t mbr;
        
    disk->ops->read(disk, 0, GPT_BLOCK_SIZE, &mbr);
    disk->ops->read(disk, 1, GPT_BLOCK_SIZE, &gpt);

    header_crc32 = gpt.header_crc32;
    gpt.header_crc32 = 0x00000000;
    return
    (
        mbr.partition1.type == 0xEE &&
        mbr.signature == MBR_BOOTSECTOR_SIG &&
        strncmp(gpt.signature, GPT_SIG, 8) == 0 &&
        calculate_crc32(&gpt, gpt.header_size) == header_crc32
    );
}