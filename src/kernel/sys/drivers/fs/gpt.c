#include "gpt.h"
#include "../../../utils/helpers/crc32.h"
#include <string.h>

uint32_t check_gpt(disk_t* disk)
{
    gpt_t gpt;
    disk->ops->read(disk, 1, sizeof(gpt_t), &gpt);
    if 
    (
        strncmp(gpt.signature, GPT_SIG, 8) == 0 &&
        calculate_crc32(&gpt, sizeof(gpt_t)) == CRC32_VALID
    )
        return gpt.gpea_num_entries;
    return 0;
}