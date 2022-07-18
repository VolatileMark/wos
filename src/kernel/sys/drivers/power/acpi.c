#include "acpi.h"
#include "../../mem/paging.h"
#include "../../../utils/log.h"
#include "../../../utils/multiboot2-utils.h"
#include "../../../utils/checksum.h"
#include <stddef.h>
#include <string.h>

#define trace_acpi(msg, ...) trace("ACPI", msg, ##__VA_ARGS__)

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

typedef struct
{
    sdt_header_t* header;
    uint64_t header_paddr;
    uint64_t pointer_size;
    uint64_t entries;
} sdt_t;

static sdt_t main_sdt;

static rsdp_descriptor_v1_t* acpi_get_rsdp(void)
{
    struct multiboot_tag_new_acpi* new;
    new = multiboot_get_tag_new_acpi();
    return (rsdp_descriptor_v1_t*) ((new == NULL) ? multiboot_get_tag_old_acpi()->rsdp : new->rsdp);
}

int acpi_init(void)
{
    uint64_t sdt_vaddr, sdt_size, sdt_addr_offset;
    sdt_header_t* mapped_sdt_header;
    rsdp_descriptor_v1_t* rsdp;
    
    rsdp = acpi_get_rsdp();
    if (strncmp(rsdp->signature, RSDP_SIG, 8))
    {
        trace_acpi("RSDP signature mismatch");
        return -1;
    }
    if (rsdp->revision == ACPI_REV_OLD && checksum8(rsdp, sizeof(rsdp_descriptor_v1_t)))
    {
        main_sdt.header_paddr = (uint64_t) rsdp->rsdt_address;
        main_sdt.pointer_size = sizeof(uint32_t);
    }
    else if (rsdp->revision >= ACPI_REV_NEW && checksum8(rsdp, sizeof(rsdp_descriptor_v2_t)))
    {
        main_sdt.header_paddr = ((rsdp_descriptor_v2_t*) rsdp)->xsdt_address;
        main_sdt.pointer_size = sizeof(uint64_t);
    }
    else
    {
        trace_acpi("Invalid ACPI revision");
        return -1;
    }
    
    sdt_addr_offset = GET_ADDR_OFFSET(main_sdt.header_paddr);
    
    mapped_sdt_header = (sdt_header_t*) (paging_map_temporary_page(main_sdt.header_paddr, PAGE_ACCESS_RO, PL0) + sdt_addr_offset);
    sdt_size = (uint64_t) mapped_sdt_header->length;
    paging_unmap_temporary_page((uint64_t) mapped_sdt_header);
    
    if (kernel_get_next_vaddr(sdt_size, &sdt_vaddr) < sdt_size)
    {
        trace_acpi("Could not find free vaddr");
        return -1;
    }
    sdt_vaddr += sdt_addr_offset;
    
    paging_map_memory(main_sdt.header_paddr, sdt_vaddr, sdt_size, PAGE_ACCESS_RO, PL0);
    main_sdt.header = (sdt_header_t*) sdt_vaddr;

    main_sdt.entries = (main_sdt.header->length - sizeof(sdt_header_t)) / main_sdt.pointer_size;
    
    info
    (
        "ACPI rev. %u.0 %cSDT located at %p, mapped at %p, has %u entries", 
        (main_sdt.pointer_size == sizeof(uint32_t)) ? 1 : rsdp->revision,
        (main_sdt.pointer_size == sizeof(uint32_t)) ? 'R' : 'X',
        main_sdt.header_paddr, 
        main_sdt.header, 
        main_sdt.entries
    );

    return 0;
}

static uint64_t acpi_get_next_sdt_entry_pointer(sdt_t* sdt, uint64_t index)
{
    uint64_t entries_array, entry_offset;
    uint64_t entry_pointer, byte_offset, byte;

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

sdt_header_t* acpi_find_table(const char* signature)
{
    sdt_header_t* header;
    uint64_t entry_paddr, entry_vaddr, entry_size, entry;
    for (entry = 0; entry < main_sdt.entries; entry++)
    {
        entry_paddr = acpi_get_next_sdt_entry_pointer(&main_sdt, entry);
        header = (sdt_header_t*) (paging_map_temporary_page(entry_paddr, PAGE_ACCESS_RO, PL0) + GET_ADDR_OFFSET(entry_paddr));
        if (strncmp(header->signature, signature, 4) == 0)
        {
            entry_size = header->length;
            paging_unmap_temporary_page((uint64_t) header);
            if (kernel_get_next_vaddr(entry_size, &entry_vaddr) < entry_size)
                return NULL;
            entry_vaddr += GET_ADDR_OFFSET(entry_paddr);
            paging_map_memory(entry_paddr, entry_vaddr, entry_size, PAGE_ACCESS_RO, PL0);
            return ((sdt_header_t*) entry_vaddr);
        }
        paging_unmap_temporary_page((uint64_t) header);
    }
    return NULL;
}
