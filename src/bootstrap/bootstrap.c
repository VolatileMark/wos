#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <mem.h>
#include "external/multiboot2.h"
#include "mem/mmap.h"
#include "mem/pfa.h"
#include "mem/paging.h"
#include "utils/constants.h"
#include "utils/elf.h"
#include "utils/asmutils.h"

void parse_multiboot_struct(uint64_t addr, uint64_t* out_size, struct multiboot_tag_module** out_module)
{
    uint64_t size = (uint64_t) *((uint32_t*) addr);
    struct multiboot_tag* tag;
    
    *out_size = size;
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
            mmap_init((struct multiboot_tag_mmap*) tag);
            break;
        case MULTIBOOT_TAG_TYPE_MODULE:
            *out_module = (struct multiboot_tag_module*) tag;
            break;
        }
    }
}

int load_elf(uint64_t start_addr, uint64_t* entry_address)
{
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*) start_addr;

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
    )
        return -1;

    *entry_address = ehdr->e_entry;

    Elf64_Phdr* phdr;
    uint64_t phdrs_offset = (((uint64_t) ehdr) + ehdr->e_phoff);
    uint64_t phdrs_total_size = ehdr->e_phentsize * ehdr->e_phnum;

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
            paging_map_memory(phdr->p_paddr, phdr->p_vaddr, phdr->p_memsz);
            memcpy((void*) phdr->p_vaddr, (void*) (((uint64_t) ehdr) + phdr->p_offset), phdr->p_filesz);
            break;
        }
    }

    return 0;
}

void bootstrap_main(uint64_t multiboot2_magic, uint64_t multiboot_struct_addr)
{
    if 
    (
        multiboot2_magic != MULTIBOOT2_BOOTLOADER_MAGIC /* Check if the magic number is correct */ ||
        multiboot_struct_addr & 0x0000000000000007 /* Check if the info struct is aligned to 4KB */
    )
        goto FAIL;

    struct multiboot_tag_module* kernel_elf;
    uint64_t multiboot_struct_size, kernel_entry;

    /* Parse the multiboot struct */
    parse_multiboot_struct(multiboot_struct_addr, &multiboot_struct_size, &kernel_elf);
    
    /* Initialize the page frame allocator */
    init_pfa();
    lock_pages(alignd((uint64_t) &_start_addr, SIZE_4KB), ceil((double) (((uint64_t) &_end_addr) - ((uint64_t) &_start_addr)) / SIZE_4KB));
    lock_pages(alignd(multiboot_struct_addr, SIZE_4KB), ceil((double) multiboot_struct_size / SIZE_4KB));

    /* Initialize paging helper */
    paging_init();

    paging_map_memory(kernel_elf->mod_start, kernel_elf->mod_start, kernel_elf->mod_end - kernel_elf->mod_start);
    
    if(load_elf(kernel_elf->mod_start, &kernel_entry))
        return;

    jump(kernel_entry);

    FAIL:
        while (1);
}