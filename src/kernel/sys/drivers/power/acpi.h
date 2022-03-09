#ifndef __ACPI_H__
#define __ACPI_H__

#include <stdint.h>

struct sdt_header
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));
typedef struct sdt_header sdt_header_t;

struct mcfg_entry
{
    uint64_t base_address;
    uint16_t pci_segment_group_num;
    uint8_t pci_start_bus_num;
    uint8_t pci_end_bus_num;
    uint32_t reserved;
} __attribute__((packed));
typedef struct mcfg_entry mcfg_entry_t;

struct mcfg
{
    sdt_header_t sdt_header;
    uint64_t reserved;
    mcfg_entry_t entries[];
} __attribute__((packed));
typedef struct mcfg mcfg_t;

#define MCFG_SIG "MCFG"
#define RSDT_SIG "RSDT"
#define XSDT_SIG "XSDT"

int acpi_init(void);
sdt_header_t* acpi_find_table(const char* signature);

#endif