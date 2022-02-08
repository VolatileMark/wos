#include "acpi.h"
#include "../mem/pfa.h"
#include "../mem/paging.h"

#define ACPI_REV_OLD 0
#define ACPI_REV_NEW 2

struct rsdp_descriptor_v1
{
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed));
typedef struct rsdp_descriptor_v1 rsdp_descriptor_v1_t;

struct rsdp_descriptor_v2
{
    rsdp_descriptor_v1_t v1;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed));
typedef struct rsdp_descriptor_v2 rsdp_descriptor_v2_t;

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

int lock_acpi_sdt_pages(uint64_t rsdp_paddr)
{
    uint64_t sdt_header_paddr, sdt_header_vaddr;
    rsdp_descriptor_v1_t* rsdp = (rsdp_descriptor_v1_t*) rsdp_paddr;
    
    if (rsdp->revision == ACPI_REV_OLD)
        sdt_header_paddr = (uint64_t) rsdp->rsdt_address;
    else if (rsdp->revision == ACPI_REV_NEW)
        sdt_header_paddr = ((rsdp_descriptor_v2_t*) rsdp)->xsdt_address;
    else
        return -1;

    sdt_header_vaddr = paging_map_temporary_page(sdt_header_paddr, PAGE_ACCESS_RO, PL0);
    lock_pages(sdt_header_paddr, (uint64_t) ((sdt_header_t*) sdt_header_vaddr)->length);
    paging_unmap_temporary_page(sdt_header_vaddr);

    return 0;
}