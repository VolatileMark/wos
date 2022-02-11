#include "acpi.h"
#include "../mem/paging.h"
#include "../utils/helpers/multiboot2-utils.h"
#include <stddef.h>
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

typedef struct
{
    sdt_header_t* header;
    uint64_t header_paddr;
    uint64_t pointer_size;
    uint64_t entries;
} sdt_t;

static sdt_t main_sdt;

static rsdp_descriptor_v1_t* get_rsdp(void)
{
    struct multiboot_tag_old_acpi* old = get_multiboot_tag_old_acpi();
    return (rsdp_descriptor_v1_t*) ((old == NULL) ? get_multiboot_tag_new_acpi()->rsdp : old->rsdp);
}

static int checksum(void* rsdp, uint64_t size)
{
    uint64_t i;
    uint8_t checksum;
    for (i = 0, checksum = 0; i < size; i++)
        checksum += ((char*) rsdp)[i];
    return (checksum == 0);
}

int init_acpi(void)
{
    uint64_t sdt_vaddr, sdt_size, sdt_addr_offset;
    sdt_header_t* mapped_sdt_header;
    rsdp_descriptor_v1_t* rsdp = get_rsdp();

    if (strncmp(rsdp->signature, RSDP_SIG, 8))
        return -1;
    if (rsdp->revision == ACPI_REV_OLD && checksum(rsdp, sizeof(rsdp_descriptor_v1_t)))
    {
        main_sdt.header_paddr = (uint64_t) rsdp->rsdt_address;
        main_sdt.pointer_size = sizeof(uint32_t);
    }
    else if (rsdp->revision >= ACPI_REV_NEW && checksum(rsdp, sizeof(rsdp_descriptor_v2_t)))
    {
        main_sdt.header_paddr = ((rsdp_descriptor_v2_t*) rsdp)->xsdt_address;
        main_sdt.pointer_size = sizeof(uint64_t);
    }
    else
        return -1;
    
    sdt_addr_offset = GET_ADDR_OFFSET(main_sdt.header_paddr);
    
    mapped_sdt_header = (sdt_header_t*) (kernel_map_temporary_page(main_sdt.header_paddr, PAGE_ACCESS_RX, PL0) + sdt_addr_offset);
    sdt_size = (uint64_t) mapped_sdt_header->length;
    kernel_unmap_temporary_page((uint64_t) mapped_sdt_header);
    
    if (kernel_get_next_vaddr(sdt_size, &sdt_vaddr) < sdt_size)
        return -1;
    sdt_vaddr += sdt_addr_offset;
    
    kernel_map_memory(main_sdt.header_paddr, sdt_vaddr, sdt_size, PAGE_ACCESS_RX, PL0);
    main_sdt.header = (sdt_header_t*) sdt_vaddr;

    main_sdt.entries = (main_sdt.header->length - sizeof(sdt_header_t)) / main_sdt.pointer_size;
    
    return 0;
}

static uint64_t get_next_entry_pointer(sdt_t* sdt, uint64_t index)
{
    uint64_t entries_array = (uint64_t) (sdt->header + 1);
    uint64_t entry_offset = index * sdt->pointer_size;
    uint64_t entry_pointer, byte_offset, byte;
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

sdt_header_t* find_acpi_table(const char* signature)
{
    sdt_header_t* header;
    uint64_t entry_paddr, entry_vaddr, entry_size, entry;
    for (entry = 0; entry < main_sdt.entries; entry++)
    {
        entry_paddr = get_next_entry_pointer(&main_sdt, entry);
        header = (sdt_header_t*) (kernel_map_temporary_page(entry_paddr, PAGE_ACCESS_RX, PL0) + GET_ADDR_OFFSET(entry_paddr));
        if (!strncmp(header->signature, signature, 4))
        {
            entry_size = header->length;
            kernel_unmap_temporary_page((uint64_t) header);
            if (kernel_get_next_vaddr(entry_size, &entry_vaddr) < entry_size)
                return NULL;
            entry_vaddr += GET_ADDR_OFFSET(entry_paddr);
            kernel_map_memory(entry_paddr, entry_vaddr, entry_size, PAGE_ACCESS_RX, PL0);
            return ((sdt_header_t*) entry_vaddr);
        }
        kernel_unmap_temporary_page((uint64_t) header);
    }
    return NULL;
}