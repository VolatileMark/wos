#include "acpi.h"
#include "../mem/paging.h"
#include "../utils/helpers/mb2utils.h"
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

static void calculate_sdt_entries(sdt_t* sdt)
{
    sdt->entries = (sdt->header->length - sizeof(sdt_header_t)) / sdt->pointer_size;
}

static int init_old_acpi(rsdp_descriptor_v1_t* rsdp)
{
    if (!checksum(rsdp, sizeof(rsdp_descriptor_v1_t)))
        return -1;
    main_sdt.header_paddr = (uint64_t) rsdp->rsdt_address;
    main_sdt.pointer_size = sizeof(uint32_t);
    calculate_sdt_entries(&main_sdt);
    return 0;
}

static int init_new_acpi(rsdp_descriptor_v2_t* rsdp)
{
    if (!checksum(rsdp, sizeof(rsdp_descriptor_v2_t)))
        return -1;
    main_sdt.header_paddr = rsdp->xsdt_address;
    main_sdt.pointer_size = sizeof(uint64_t);
    return 0;
}

int init_acpi(void)
{
    uint64_t sdt_vaddr, sdt_size;
    sdt_header_t* mapped_sdt_header;
    rsdp_descriptor_v1_t* rsdp = get_rsdp();

    if 
    (
        strncmp(rsdp->signature, RSDP_SIG, 8) ||
        (rsdp->revision == ACPI_REV_OLD && init_old_acpi(rsdp)) ||
        (rsdp->revision == ACPI_REV_NEW && init_new_acpi((rsdp_descriptor_v2_t*) rsdp))
    )
        return -1;
    
    mapped_sdt_header = (sdt_header_t*) kernel_map_temporary_page(main_sdt.header_paddr, PAGE_ACCESS_RO, PL0);
    sdt_size = (uint64_t) mapped_sdt_header->length;
    kernel_unmap_temporary_page((uint64_t) mapped_sdt_header);
    if (kernel_get_next_vaddr(sdt_size, &sdt_vaddr) < sdt_size)
        return -1;
    kernel_map_memory(main_sdt.header_paddr, sdt_vaddr, sdt_size, PAGE_ACCESS_RO, PL0);
    main_sdt.header = (sdt_header_t*) sdt_vaddr;

    calculate_sdt_entries(&main_sdt);
    
    return 0;
}

sdt_header_t* find_acpi_table(const char* signature)
{
    sdt_header_t* header;
    uint64_t i;
    for (i = 0; i < main_sdt.entries; i++)
    {
        header = (sdt_header_t*) (((uint64_t) (main_sdt.header + 1)) + (i * main_sdt.pointer_size));
        if (!strncmp(header->signature, signature, 4))
            return header;
    }
    return NULL;
}