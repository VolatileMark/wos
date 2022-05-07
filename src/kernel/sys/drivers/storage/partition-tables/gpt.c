#include "gpt.h"
#include "mbr.h"
#include "../../../../headers/constants.h"
#include "../../../../utils/crc32.h"
#include "../../../../utils/alloc.h"
#include "../../../../utils/log.h"
#include <string.h>
#include <stddef.h>

#define trace_gpt(msg, ...) trace("GPTU", msg, ##__VA_ARGS__)

typedef struct gpea_chain_entry
{
    struct gpea_chain_entry* next;
    gpt_entry_t entry;
} gpea_chain_entry_t;

void gpt_load(drive_t* drive, gpt_t* gpt)
{
    drivefs_read(drive, 1, sizeof(gpt_t), gpt);
}

int gpt_check(drive_t* drive)
{
    uint32_t header_crc32;
    gpt_t gpt;
    mbr_t mbr;
        
    mbr_load(drive, &mbr);
    gpt_load(drive, &gpt);

    header_crc32 = gpt.header_crc32;
    gpt.header_crc32 = 0x00000000;
    return
    (
        mbr.partition1.type == MBR_PART_GPT &&
        mbr.signature == MBR_BOOTSECTOR_SIG &&
        strncmp(gpt.signature, GPT_SIG, 8) == 0 &&
        crc32_calculate(&gpt, gpt.header_size) == header_crc32
    );
    return 0;
}

static int gpt_guidcmp(guid_t* guid1, guid_t guid2)
{
    return guidcmp(guid1, &guid2);
}

int gpt_find_partitions(drive_t* drive, gpt_t* gpt)
{
    uint64_t i, offset, lba;
    uint8_t sector[SIZE_SECTOR];
    gpt_entry_t* gpt_entry;
    gpea_chain_entry_t* chain;
    gpea_chain_entry_t* next;

    for 
    (
        i = 0, lba = gpt->gpea_lba, next = NULL; 
        i < gpt->gpea_num_entries; 
        i++, lba++
    )
    {
        drivefs_read(drive, lba, SIZE_SECTOR, sector);
        for (offset = 0; offset < SIZE_SECTOR; offset += gpt->gpea_entry_size)
        {
            gpt_entry = (gpt_entry_t*)(sector + offset);
            if (gpt_guidcmp(&gpt_entry->partition_guid, GPT_ENTRY_NULL))
                goto END;
            chain = malloc(sizeof(gpea_chain_entry_t));
            if (chain == NULL)
            {
                trace_gpt("Could not allocate space for GPT Entry Array link");
                goto END;
            }
            chain->entry = *gpt_entry;
            chain->next = next;
            next = chain;
        }
    }

    END:

    drive->partitions = malloc(sizeof(drive_partition_t) * i);
    if (drive->partitions == NULL)
        trace_gpt("Could not allocate space for partition array");
    i = 0;
    while (chain != NULL)
    {
        next = chain->next;
        if (drive->partitions != NULL)
        {
            drive->partitions[i].lba_start = chain->entry.start_lba;
            drive->partitions[i].lba_end = chain->entry.end_lba;
            drive->partitions[i].sectors = chain->entry.end_lba - chain->entry.start_lba;
            drive->partitions[i].flags = chain->entry.attributes;
            ++i;
        }
        free(chain);
        chain = next;
    }

    return i;
}