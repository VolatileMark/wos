#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <mem.h>
#include "utils/multiboot2.h"
#include "mem/mmap.h"
#include "mem/pfa.h"
#include "mem/paging.h"
#include "utils/constants.h"
#include "utils/elf.h"
#include "utils/bitmap.h"
#include "utils/ustar.h"

static int parse_multiboot_struct(uint64_t addr, uint64_t* out_size, struct multiboot_tag_module** out_module)
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
            if (init_mmap((struct multiboot_tag_mmap*) tag))
                return -1;
            break;
        case MULTIBOOT_TAG_TYPE_MODULE:
            *out_module = (struct multiboot_tag_module*) tag;
            break;
        }
    }

    return 0;
}

static int load_elf(uint64_t start_addr, uint64_t* entry_address, uint64_t* elf_start_addr, uint64_t* elf_end_addr)
{
    uint64_t phdrs_offset, phdrs_total_size;
    Elf64_Phdr* phdr;
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
    *elf_start_addr = (uint64_t) -1;
    *elf_end_addr = 0;

    
    phdrs_offset = (((uint64_t) ehdr) + ehdr->e_phoff);
    phdrs_total_size = ehdr->e_phentsize * ehdr->e_phnum;

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
            if (*elf_start_addr > phdr->p_paddr)
                *elf_start_addr = phdr->p_paddr;
            if (*elf_end_addr < phdr->p_paddr + phdr->p_memsz)
                *elf_end_addr = phdr->p_paddr + phdr->p_memsz;
            paging_unmap_memory(phdr->p_paddr, phdr->p_memsz);
            paging_map_memory(phdr->p_paddr, phdr->p_vaddr, phdr->p_memsz, PAGE_ACCESS_RW, PL0);
            memcpy((void*) phdr->p_vaddr, (void*) (((uint64_t) ehdr) + phdr->p_offset), phdr->p_filesz);
            break;
        }
    }

    return 0;
}

static bitmap_t* free_useless_pages(struct multiboot_tag_module* initrd)
{
    page_table_t pml4, pdp, pd;
    page_table_entry_t entry;
    uint64_t paddr;
    uint64_t pml4_idx, pdp_idx, pd_idx;
    bitmap_t* page_bitmap;

    page_bitmap = get_page_bitmap();

    free_pages(alignd((uint64_t) &_start_addr, SIZE_4KB), ceil((double) (((uint64_t) &_end_addr) - ((uint64_t) &_start_addr)) / SIZE_4KB));
    free_pages(alignd(initrd->mod_start, SIZE_4KB), ceil((double) (initrd->mod_end - initrd->mod_start) / SIZE_4KB));

    lock_pages(alignd((uint64_t) page_bitmap->buffer, SIZE_4KB), ceil((double) page_bitmap->size / SIZE_4KB));
    lock_page(get_current_pml4_paddr());

    pml4 = (page_table_t) PML4_VADDR;
    for (pml4_idx = 0; pml4_idx < MAX_PAGE_TABLE_ENTRIES; pml4_idx++)
    {
        entry = pml4[pml4_idx];
        if (entry.present)
        {
            paddr = get_pte_address(&entry);
            lock_page(paddr);
            pdp = (page_table_t) paging_map_temporary_page(paddr, PAGE_ACCESS_RO, PL0);
            for (pdp_idx = 0; pdp_idx < MAX_PAGE_TABLE_ENTRIES; pdp_idx++)
            {
                entry = pdp[pdp_idx];
                if (entry.present)
                {
                    paddr = get_pte_address(&entry);
                    lock_page(paddr);
                    pd = (page_table_t) paging_map_temporary_page(paddr, PAGE_ACCESS_RO, PL0);
                    for (pd_idx = 0; pd_idx < MAX_PAGE_TABLE_ENTRIES; pd_idx++)
                    {
                        entry = pd[pd_idx];
                        if (entry.present)
                            lock_page(get_pte_address(&entry));
                    }
                    paging_unmap_temporary_page((uint64_t) pd);
                }
            }
            paging_unmap_temporary_page((uint64_t) pdp);
        }
    }

    return page_bitmap;
}

static void relocate_initrd(struct multiboot_tag_module* initrd)
{
    uint64_t size = initrd->mod_end - initrd->mod_start;
    paging_map_memory(initrd->mod_start, initrd->mod_start, size, PAGE_ACCESS_RO, PL0);
    paging_map_memory(INITRD_RELOC_ADDR, INITRD_RELOC_ADDR, size, PAGE_ACCESS_RW, PL0);
    memcpy((void*) INITRD_RELOC_ADDR, (void*) ((uint64_t) initrd->mod_start), size);
    paging_unmap_memory(initrd->mod_start, size);
    initrd->mod_start = INITRD_RELOC_ADDR;
    initrd->mod_end = INITRD_RELOC_ADDR + size;
    lock_pages(initrd->mod_start, ceil((double) size / SIZE_4KB));
}

void bootstrap_main(uint64_t multiboot2_magic, uint64_t multiboot_struct_addr)
{
    struct multiboot_tag_module* initrd;
    uint64_t multiboot_struct_size;
    uint64_t kernel_elf_addr, kernel_entry, kernel_start_paddr, kernel_end_paddr;
    uint64_t init_exec_file_size, init_exec_file_addr, init_exec_file_new_addr;
    uint64_t fsrv_exec_file_size, fsrv_exec_file_addr, fsrv_exec_file_new_addr;
    bitmap_t* new_bitmap;

    if 
    (
        multiboot2_magic != MULTIBOOT2_BOOTLOADER_MAGIC /* Check if the magic number is correct */ ||
        multiboot_struct_addr & 0x0000000000000007 /* Check if the info struct is aligned to 4KB */
    )
        return;

    /* Parse the multiboot struct */
    if (parse_multiboot_struct(multiboot_struct_addr, &multiboot_struct_size, &initrd))
        return;
    
    /* Initialize the page frame allocator */
    init_pfa();
    lock_pages(alignd((uint64_t) &_start_addr, SIZE_4KB), ceil((double) (((uint64_t) &_end_addr) - ((uint64_t) &_start_addr)) / SIZE_4KB));
    lock_pages(alignd(multiboot_struct_addr, SIZE_4KB), ceil((double) multiboot_struct_size / SIZE_4KB));
    lock_pages(alignd(initrd->mod_start, SIZE_4KB), ceil((double) (initrd->mod_end - initrd->mod_start) / SIZE_4KB));

    /* Initialize paging helper */
    init_paging();

    /* Relocate initrd */
    relocate_initrd(initrd);
    ustar_set_address(initrd->mod_start);
    
    ustar_lookup(KERNEL_ELF_PATH, &kernel_elf_addr);
    if (load_elf(kernel_elf_addr, &kernel_entry, &kernel_start_paddr, &kernel_end_paddr))
        return;
    lock_pages(alignd(kernel_start_paddr, SIZE_4KB), ceil((double) (kernel_end_paddr - kernel_start_paddr) / SIZE_4KB));
    
    /* Relocate init executable */
    {
        init_exec_file_size = ustar_lookup(INIT_EXEC_FILE_PATH, &init_exec_file_addr);
        init_exec_file_new_addr = alignu(kernel_end_paddr, SIZE_4KB);
        paging_map_memory(init_exec_file_addr, init_exec_file_addr, init_exec_file_size, PAGE_ACCESS_RO, PL0);
        paging_map_memory(init_exec_file_new_addr, init_exec_file_new_addr, init_exec_file_size, PAGE_ACCESS_RW, PL0);
        memcpy((void*) init_exec_file_new_addr, (void*) init_exec_file_addr, init_exec_file_size);
        lock_pages(alignd(init_exec_file_new_addr, SIZE_4KB), ceil((double) (init_exec_file_new_addr + init_exec_file_size) / SIZE_4KB));
    }

    /* Relocate fsrv executable */
    {
        fsrv_exec_file_size = ustar_lookup(FSRV_EXEC_FILE_PATH, &fsrv_exec_file_addr);
        fsrv_exec_file_new_addr = alignu(init_exec_file_new_addr, SIZE_4KB);
        paging_map_memory(fsrv_exec_file_addr, fsrv_exec_file_addr, fsrv_exec_file_size, PAGE_ACCESS_RO, PL0);
        paging_map_memory(fsrv_exec_file_new_addr, fsrv_exec_file_new_addr, fsrv_exec_file_size, PAGE_ACCESS_RW, PL0);
        memcpy((void*) fsrv_exec_file_new_addr, (void*) fsrv_exec_file_addr, fsrv_exec_file_size);
        lock_pages(alignd(fsrv_exec_file_new_addr, SIZE_4KB), ceil((double) (fsrv_exec_file_new_addr + fsrv_exec_file_size) / SIZE_4KB));
    }

    paging_unmap_memory(initrd->mod_start, initrd->mod_end - initrd->mod_start);

    new_bitmap = free_useless_pages(initrd);

    void (*kernel_main)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, bitmap_t*) = ((__attribute__((sysv_abi)) void (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, bitmap_t*)) kernel_entry);
    kernel_main(multiboot_struct_addr, init_exec_file_new_addr, init_exec_file_size, fsrv_exec_file_new_addr, fsrv_exec_file_size, new_bitmap);
}