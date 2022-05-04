#include "process.h"
#include "vfs/vfs.h"
#include "../headers/elf.h"
#include "../utils/alloc.h"
#include "../utils/log.h"
#include "../sys/mem/pfa.h"
#include "../sys/mem/paging.h"
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <mem.h>

#define trace_process(msg, ...) trace("PROC", msg, ##__VA_ARGS__)

int process_request_memory(process_t* ps, uint64_t size, uint64_t hint, page_access_type_t access)
{
    uint64_t vaddr, paddr, pages;
    memory_segments_list_entry_t* entry;
    
    pages = ceil((double) size / SIZE_4KB);
    if (pml4_get_next_vaddr(ps->pml4, hint, size, &vaddr) < size)
        return -1;

    entry = malloc(sizeof(memory_segments_list_entry_t));
    if (entry == NULL)
        return -1;
    
    paddr = pfa_request_pages(pages);
    if 
    (
        paddr == 0 ||
        pml4_map_memory(ps->pml4, paddr, vaddr, size, access, PL3) < size
    )
    {
        free(entry);
        pml4_unmap_memory(ps->pml4, vaddr, size);
        return -1;
    }

    entry->paddr = paddr;
    entry->pages = pages;

    if (ps->mem.tail == NULL)
        ps->mem.head = entry;
    else
        ps->mem.tail->next = entry;
    ps->mem.tail = entry;
    
    return 0;
}

static int process_create_pml4(process_t* ps)
{
    ps->pml4 = aligned_alloc(SIZE_4KB, SIZE_4KB);
    if (ps->pml4 == NULL)
    {
        free(ps->pml4);
        return -1;
    }
    ps->pml4_paddr = kernel_get_paddr((uint64_t) ps->pml4);
    return 0;
}

static int process_set_exec(process_t* ps, const char* path)
{
    ps->exec = malloc(strlen(path));
    if (ps->exec == NULL)
        return -1;
    strcpy((char*) ps->exec, path);
    return 0;
}

static int process_load_executable(process_t* ps)
{
    vnode_t file;
    vattribs_t attr;
    uint8_t* buffer;
    Elf64_Ehdr* ehdr;

    if 
    (
        vfs_lookup(ps->exec, &file) ||
        vfs_get_attribs(&file, &attr) ||
        attr.size == 0
    )
        return -1;    

    buffer = malloc(attr.size);
    if (buffer == NULL)
        return -1;

    if (vfs_read(&file, buffer, attr.size))
    {
        free(buffer);
        return -1;
    }

    ehdr = (Elf64_Ehdr*) buffer;
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
    {
        free(buffer);
        return -1;
    }

    return 0;
}

process_t* process_create(const char *path, uint64_t pid)
{
    process_t* ps = calloc(1, sizeof(process_t));
    if (ps == NULL)
        return NULL;

    ps->pid = pid;
    if (process_create_pml4(ps))
    {
        trace_process("Failed to create process PML4");
        free(ps);
        return NULL;
    }

    if (process_set_exec(ps, path))
    {
        trace_process("Failed to set process executable path");
        free(ps);
        return NULL;
    }

    if (process_load_executable(ps))
    {
        trace_process("Failed to load process executable");
        free(ps);
        return NULL;
    }

    return ps;
}