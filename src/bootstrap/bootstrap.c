#include <stddef.h>
#include <math.h>
#include <mem.h>
#include "mem/mmap.h"
#include "mem/pfa.h"
#include "mem/paging.h"
#include "utils/elf.h"
#include "sys/acpi.h"

static int parse_multiboot_struct(uint64_t addr, uint64_t* out_size, struct multiboot_tag_old_acpi** out_rsdp, struct multiboot_tag_module** out_module)
{
    uint64_t size;
    struct multiboot_tag* tag;
    
    size = (uint64_t) *((uint32_t*) addr);

    *out_size = size;
    *out_rsdp = NULL;
    *out_module = NULL;
    
    for 
    (
        tag = (struct multiboot_tag*) alignu(addr + sizeof(struct multiboot_tag), MULTIBOOT_TAG_ALIGN);
        ((uint64_t) tag) < (addr + size) && tag->type != MULTIBOOT_TAG_TYPE_END;
        tag = (struct multiboot_tag*) alignu(((uint64_t) tag) + tag->size, MULTIBOOT_TAG_ALIGN)
    )
    {
        switch (tag->type)
        {
        case MULTIBOOT_TAG_TYPE_MMAP:
            if (mmap_init((struct multiboot_tag_mmap*) tag)) { return -1; }
            break;
        case MULTIBOOT_TAG_TYPE_MODULE:
            *out_module = (struct multiboot_tag_module*) tag;
            break;
        case MULTIBOOT_TAG_TYPE_ACPI_NEW:
        case MULTIBOOT_TAG_TYPE_ACPI_OLD:
            *out_rsdp = (struct multiboot_tag_old_acpi*) tag;
            break;
        }
    }

    return 0;
}

static int load_elf(uint64_t start_addr, uint64_t size, uint64_t* entry_address)
{
    uint64_t phdrs_offset, phdrs_total_size, paddr;
    Elf64_Phdr* phdr;
    Elf64_Ehdr* ehdr;
    
    ehdr = (Elf64_Ehdr*) start_addr;

    if 
    (
        ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
        ehdr->e_ident[EI_OSABI] != ELFOSABI_SYSV ||
        ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
        ehdr->e_machine != EM_X86_64 ||
        ehdr->e_type != ET_EXEC
    ) { return -1; }

    phdrs_offset = (((uint64_t) ehdr) + ehdr->e_phoff);
    phdrs_total_size = ehdr->e_phentsize * ehdr->e_phnum;

    *entry_address = ehdr->e_entry;

    for
    (
        phdr = (Elf64_Phdr*) phdrs_offset;
        phdr < (Elf64_Phdr*) (((uint64_t) phdrs_offset) + phdrs_total_size);
        phdr = (Elf64_Phdr*) (((uint64_t) phdr) + ehdr->e_phentsize)
    )
    {
        switch (phdr->p_type)
        {
        case PT_LOAD:
            paddr = pfa_request_pages(ceil((double) phdr->p_memsz / SIZE_4KB));
            paging_map_memory(paddr, phdr->p_vaddr, phdr->p_memsz, PAGE_ACCESS_RW, PL0);
            memcpy((void*) phdr->p_vaddr, (void*) (((uint64_t) ehdr) + phdr->p_offset), phdr->p_filesz);
            break;
        }
    }


    return 0;
}

static bitmap_t* free_useless_pages(uint64_t multiboot_struct_addr, uint64_t multiboot_struct_size, struct multiboot_tag_module* kernel_elf)
{
    page_table_t pml4, pdp, pd;
    page_table_entry_t entry;
    uint64_t paddr;
    uint64_t pml4_idx, pdp_idx, pd_idx;
    bitmap_t* page_bitmap;

    page_bitmap = pfa_get_page_bitmap();

    pfa_free_pages(alignd((uint64_t) &_start_addr, SIZE_4KB), ceil((double) (((uint64_t) &_end_addr) - ((uint64_t) &_start_addr)) / SIZE_4KB));
    pfa_free_pages(alignd(kernel_elf->mod_start, SIZE_4KB), ceil((double) (kernel_elf->mod_end - kernel_elf->mod_start) / SIZE_4KB));

    pfa_lock_pages(alignd(multiboot_struct_addr, SIZE_4KB), ceil((double) multiboot_struct_size / SIZE_4KB));
    pfa_lock_pages(alignd((uint64_t) page_bitmap->buffer, SIZE_4KB), ceil((double) page_bitmap->size / SIZE_4KB));
    pfa_lock_page(pml4_get_current_paddr());

    pml4 = (page_table_t) PML4_VADDR;
    for (pml4_idx = 0; pml4_idx < PT_MAX_ENTRIES; pml4_idx++)
    {
        entry = pml4[pml4_idx];
        if (entry.present)
        {
            paddr = pte_get_address(&entry);
            pfa_lock_page(paddr);
            pdp = (page_table_t) paging_map_temporary_page(paddr, PAGE_ACCESS_RO, PL0);
            for (pdp_idx = 0; pdp_idx < PT_MAX_ENTRIES; pdp_idx++)
            {
                entry = pdp[pdp_idx];
                if (entry.present)
                {
                    paddr = pte_get_address(&entry);
                    pfa_lock_page(paddr);
                    pd = (page_table_t) paging_map_temporary_page(paddr, PAGE_ACCESS_RO, PL0);
                    for (pd_idx = 0; pd_idx < PT_MAX_ENTRIES; pd_idx++)
                    {
                        entry = pd[pd_idx];
                        if (entry.present) { pfa_lock_page(pte_get_address(&entry)); }
                    }
                    paging_unmap_temporary_page((uint64_t) pd);
                }
            }
            paging_unmap_temporary_page((uint64_t) pdp);
        }
    }

    return page_bitmap;
}

uint64_t align_multiboot_struct
(
    uint64_t current_addr, 
    uint64_t size, 
    uint64_t boundary, 
    struct multiboot_tag_old_acpi** rsdp_tag, 
    struct multiboot_tag_module** kernel_elf
)
{
    uint64_t new_addr, tmp_size;
    uint8_t* src;
    uint8_t* dst;

    new_addr = alignu(maxu(current_addr, (*kernel_elf)->mod_end), boundary);
    src = (uint8_t*) (current_addr + (size - 1));
    dst = (uint8_t*) (new_addr + (size - 1));
    tmp_size = size;

    paging_map_memory(new_addr, new_addr, size, PAGE_ACCESS_RW, PL0);
    for (; tmp_size > 0; tmp_size--) { *dst-- = *src--; }

    *rsdp_tag = (struct multiboot_tag_old_acpi*) (new_addr + (((uint64_t) *rsdp_tag) - current_addr));
    *kernel_elf = (struct multiboot_tag_module*) (new_addr + (((uint64_t) *kernel_elf) - current_addr));

    return new_addr;
}

void bootstrap_main(uint64_t multiboot2_magic, uint64_t multiboot_struct_addr)
{
    struct multiboot_tag_module* kernel_elf;
    struct multiboot_tag_old_acpi* rsdp_tag;
    uint64_t kernel_entry;
    uint64_t multiboot_struct_size, multiboot_struct_new_addr;
    uint64_t rsdp_size;
    bitmap_t* new_bitmap;

    if 
    (
        multiboot2_magic != MULTIBOOT2_BOOTLOADER_MAGIC /* Check if the magic number is correct */ ||
        multiboot_struct_addr & 0x0000000000000007 /* Check if the info struct is aligned to 4KB */
    ) { return; }

    /* Parse the multiboot struct */
    if 
    (
        parse_multiboot_struct(multiboot_struct_addr, &multiboot_struct_size, &rsdp_tag, &kernel_elf) ||
        multiboot_struct_size == 0 ||
        rsdp_tag == NULL ||
        kernel_elf == NULL
    ) { return; }

    /* Initialize the page frame allocator */
    pfa_init();

    /* Initialize paging helper */
    paging_init();

    pfa_lock_pages(alignd((uint64_t) &_start_addr, SIZE_4KB), ceil((double) (((uint64_t) &_end_addr) - ((uint64_t) &_start_addr)) / SIZE_4KB));
    pfa_lock_pages(alignd(kernel_elf->mod_start, SIZE_4KB), ceil((double) (kernel_elf->mod_end - kernel_elf->mod_start) / SIZE_4KB));

    if (acpi_lock_sdt_pages((uint64_t) rsdp_tag->rsdp, &rsdp_size)) { return; }
    
    multiboot_struct_new_addr = align_multiboot_struct
    (
        multiboot_struct_addr, 
        multiboot_struct_size, 
        SIZE_4KB, 
        &rsdp_tag,
        &kernel_elf
    );
    pfa_lock_pages(alignd(multiboot_struct_new_addr, SIZE_4KB), ceil((double) multiboot_struct_size / SIZE_4KB));
    paging_unmap_memory(multiboot_struct_addr, multiboot_struct_new_addr - multiboot_struct_addr);
    paging_map_memory(kernel_elf->mod_start, kernel_elf->mod_start, kernel_elf->mod_end - kernel_elf->mod_start, PAGE_ACCESS_RW, PL0);

    if 
    (
        load_elf
        (
            kernel_elf->mod_start, 
            kernel_elf->mod_end - kernel_elf->mod_start, 
            &kernel_entry
        )
    ) { return; }

    paging_unmap_memory(kernel_elf->mod_start, kernel_elf->mod_end - kernel_elf->mod_start);

    new_bitmap = free_useless_pages(multiboot_struct_addr, multiboot_struct_size, kernel_elf);

    void (*kernel_main)(uint64_t, bitmap_t*) = ((__attribute__((sysv_abi)) void (*)(uint64_t, bitmap_t*)) kernel_entry);
    kernel_main(multiboot_struct_new_addr, new_bitmap);
}
