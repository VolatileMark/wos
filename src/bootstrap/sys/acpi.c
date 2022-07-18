#include "acpi.h"
#include "../mem/pfa.h"
#include "../mem/paging.h"
#include <math.h>
#include <string.h>

#define ACPI_REV_OLD 0
#define ACPI_REV_NEW 2
#define RSDP_SIG "RSD PTR "

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

typedef struct
{
    sdt_header_t* header;
    uint64_t header_paddr;
    uint64_t pointer_size;
    uint64_t entries;
} sdt_t;

static int acpi_checksum(void* rsdp, uint64_t size)
{
    uint64_t i;
    uint8_t checksum;
    for (i = 0, checksum = 0; i < size; i++) { checksum += ((char*) rsdp)[i]; }
    return (checksum == 0);
}

static uint64_t acpi_get_next_sdt_entry_pointer(sdt_t* sdt, uint64_t index)
{
    uint64_t entries_array, entry_offset, entry_pointer, byte_offset, byte;

    entries_array = (uint64_t) (sdt->header + 1);
    entry_offset = index * sdt->pointer_size;

    for 
    (
        byte = sdt->pointer_size, entry_pointer = 0; 
        byte > 0;
        byte--
    )
    {
        byte_offset = byte - 1;
        entry_pointer |= *((uint8_t*) (entries_array + entry_offset + byte_offset)) << (8 * byte_offset);
    }

    return entry_pointer;
}

int acpi_lock_sdt_pages(uint64_t rsdp_start_paddr, uint64_t* rsdp_size)
{
    sdt_t sdt;
    sdt_header_t* highest_header;
    uint64_t min_addr, max_addr, addr, entry;
    rsdp_descriptor_v1_t* rsdp;
    
    rsdp = (rsdp_descriptor_v1_t*) rsdp_start_paddr;
    
    if (strncmp(rsdp->signature, RSDP_SIG, 8)) { return -1; }
    if (rsdp->revision == ACPI_REV_OLD && acpi_checksum(rsdp, sizeof(rsdp_descriptor_v1_t)))
    {
        sdt.header_paddr = (uint64_t) rsdp->rsdt_address;
        sdt.pointer_size = sizeof(uint32_t);
        *rsdp_size = sizeof(rsdp_descriptor_v1_t);
    }
    else if (rsdp->revision >= ACPI_REV_NEW && acpi_checksum(rsdp, sizeof(rsdp_descriptor_v2_t)))
    {
        sdt.header_paddr = ((rsdp_descriptor_v2_t*) rsdp)->xsdt_address;
        sdt.pointer_size = sizeof(uint64_t);
        *rsdp_size = sizeof(rsdp_descriptor_v2_t);
    }
    else { return -1; }

    sdt.header = (sdt_header_t*) (paging_map_temporary_page(sdt.header_paddr, PAGE_ACCESS_RO, PL0) + GET_ADDR_OFFSET(sdt.header_paddr));
    sdt.entries = (sdt.header->length - sizeof(sdt_header_t)) / sdt.pointer_size;
    
    for 
    (
        entry = 0, min_addr = sdt.header_paddr, max_addr = sdt.header_paddr; 
        entry < sdt.entries; 
        entry++
    )
    {
        addr = acpi_get_next_sdt_entry_pointer(&sdt, entry);
        if (addr > max_addr) { max_addr = addr; }
        if (addr < min_addr) { min_addr = addr; }
    }
    paging_unmap_temporary_page((uint64_t) sdt.header);
    
    highest_header = (sdt_header_t*) (paging_map_temporary_page(max_addr, PAGE_ACCESS_RO, PL0) + GET_ADDR_OFFSET(max_addr));
    max_addr += highest_header->length;
    pfa_lock_pages(min_addr, ceil((double) (max_addr - min_addr) / SIZE_4KB));
    paging_unmap_temporary_page((uint64_t) highest_header);

    return 0;
}
