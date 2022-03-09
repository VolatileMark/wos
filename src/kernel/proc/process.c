#include "process.h"
#include "scheduler.h"
#include "vfs/vfs.h"
#include "vfs/vnode.h"
#include "vfs/vattribs.h"
#include "../kernel.h"
#include "../mem/pfa.h"
#include "../mem/paging.h"
#include "../utils/elf.h"
#include "../utils/constants.h"
#include "../utils/macros.h"
#include "../utils/alloc.h"
#include "../sys/cpu/gdt.h"
#include <stddef.h>
#include <mem.h>
#include <math.h>
#include <string.h>

static void process_init(process_t* ps, uint64_t pid)
{
    ps->pid = pid;
    ps->parent_pid = 0;
    
    ps->pml4 = NULL;
    ps->pml4_paddr = 0;
    
    ps->kernel_stack_start_vaddr = 0;
    ps->code_start_vaddr = 0;
    ps->args_start_vaddr = 0;
    ps->stack_start_vaddr = PROC_DEFAULT_STACK_VADDR;
    ps->mapping_start_vaddr = 0;
    
    ps->kernel_stack_pages.head = NULL;
    ps->kernel_stack_pages.tail = NULL;
    
    ps->code_pages.head = NULL;
    ps->code_pages.tail = NULL;
    
    ps->stack_pages.head = NULL;
    ps->stack_pages.tail = NULL;
    
    ps->mapped_pages.head = NULL;
    ps->mapped_pages.tail = NULL;
    
    ps->args_pages.head = NULL;
    ps->args_pages.tail = NULL;

    memset(&ps->flags, 0, sizeof(process_flags_t));
    memset(ps->fds, 0, PROC_MAX_FDS * sizeof(file_descriptor_t));
    memset(&ps->user_mode, 0, sizeof(cpu_state_t));
    memset(&ps->current, 0, sizeof(cpu_state_t));

    ps->user_mode.stack.rflags = PROC_DEFAULT_RFLAGS;
    ps->user_mode.stack.cs = gdt_get_user_cs() | PL3;
    ps->user_mode.stack.ss = gdt_get_user_ds() | PL3;
}

static int process_init_pml4(process_t* ps)
{
    uint64_t paddr, vaddr;

    paddr = pfa_request_page();
    if 
    (
        paddr == 0 || 
        kernel_get_next_vaddr(SIZE_4KB, &vaddr) < SIZE_4KB ||
        kernel_map_memory(paddr, vaddr, SIZE_4KB, PAGE_ACCESS_RW, PL0) < SIZE_4KB
    )
        return -1;

    ps->pml4 = (page_table_t) vaddr;
    ps->pml4_paddr = paddr;
    memset(ps->pml4, 0, SIZE_4KB);

    return 0;
}

static int process_load_code(process_t* ps, const char* path)
{
    uint64_t phdrs_offset, phdrs_total_size;
    uint64_t copy_tmp_vaddr;
    uint64_t highest_vaddr, section_highest_vaddr, lowest_vaddr, section_lowest_vaddr;
    uint64_t alloc_paddr, alloc_pages;
    pages_list_entry_t* code_pages;
    Elf64_Phdr* phdr;
    Elf64_Ehdr* ehdr;
    vnode_t node;
    vattribs_t attribs;
    void* buffer;

    vfs_lookup(path, &node);
    vfs_get_attribs(&node, &attribs);
    
    buffer = malloc(attribs.size);
    if (buffer == NULL)
        return -1;
    
    vfs_read(&node, buffer, attribs.size);

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
        return -1;

    phdrs_offset = (((uint64_t) ehdr) + ehdr->e_phoff);
    phdrs_total_size = ehdr->e_phentsize * ehdr->e_phnum;
    highest_vaddr = 0;
    lowest_vaddr = (uint64_t) -1;

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
            alloc_pages = ceil((double) phdr->p_memsz / SIZE_4KB);
            alloc_paddr = pfa_request_pages(alloc_pages);
            
            if (kernel_get_next_vaddr(phdr->p_memsz, &copy_tmp_vaddr) < phdr->p_memsz)
            {
                free(buffer);
                return -1;
            }
            if 
            (
                kernel_map_memory(alloc_paddr, copy_tmp_vaddr, phdr->p_filesz, PAGE_ACCESS_RW, PL0) < phdr->p_filesz ||
                pml4_map_memory(ps->pml4, alloc_paddr, phdr->p_vaddr, phdr->p_memsz, PAGE_ACCESS_WX, PL3) < phdr->p_memsz
            )
            {
                kernel_unmap_memory(copy_tmp_vaddr, phdr->p_filesz);
                free(buffer);
                return -1;
            }
            memcpy((void*) copy_tmp_vaddr, (void*) (((uint64_t) ehdr) + phdr->p_offset), phdr->p_filesz);
            kernel_unmap_memory(copy_tmp_vaddr, phdr->p_filesz);
            
            code_pages = malloc(sizeof(pages_list_entry_t));
            if (code_pages == NULL)
            {
                free(buffer);
                return -1;
            }

            code_pages->next = NULL;
            code_pages->pages = alloc_paddr;
            code_pages->pages = alloc_pages;

            if (ps->code_pages.tail == NULL)
                ps->code_pages.head = code_pages;
            else
                ps->code_pages.tail->next = code_pages;
            ps->code_pages.tail = code_pages;
            
            section_lowest_vaddr = phdr->p_vaddr;
            section_highest_vaddr = section_lowest_vaddr + phdr->p_memsz;
            if (highest_vaddr < section_highest_vaddr)
                highest_vaddr = alignu(section_highest_vaddr, SIZE_4KB);
            if (lowest_vaddr > section_lowest_vaddr)
                lowest_vaddr = alignd(section_lowest_vaddr, SIZE_4KB);
            
            break;
        }
    }

    free(buffer);
    
    ps->code_start_vaddr = lowest_vaddr;
    ps->user_mode.stack.rip = ehdr->e_entry;
    ps->mapping_start_vaddr = highest_vaddr;

    return 0;
}

static int process_init_stack(process_t* ps)
{
    uint64_t paddr, bytes, pages, size;
    pages_list_entry_t* stack_pages;

    pages = ceil((double) PROC_INITIAL_STACK_SIZE / SIZE_4KB);
    paddr = pfa_request_pages(PROC_INITIAL_STACK_SIZE);
    if (paddr == 0)
        return -1;
    
    bytes = pages * SIZE_4KB;
    size = pml4_map_memory(ps->pml4, paddr, PROC_DEFAULT_STACK_VADDR, bytes, PAGE_ACCESS_RW, PL3);
    if (size < bytes)
    {
        pfa_free_pages(paddr, pages);
        return -1;
    }

    stack_pages = malloc(sizeof(pages_list_entry_t));
    if (stack_pages == NULL)
    {
        pfa_free_pages(paddr, pages);
        return -1;
    }
    
    stack_pages->paddr = paddr;
    stack_pages->pages = pages;
    stack_pages->next = NULL;

    ps->stack_pages.head = stack_pages;
    ps->stack_pages.tail = stack_pages;
    ps->stack_start_vaddr = PROC_DEFAULT_STACK_VADDR;
    ps->user_mode.stack.rsp = PROC_INITIAL_RSP;

    return 0;
}

static int process_init_kernel_stack(process_t* ps)
{
    uint64_t pages, bytes, vaddr, paddr, size;
    pages_list_entry_t* kernel_stack_pages;

    bytes = PROC_INITIAL_STACK_SIZE;
    size = kernel_get_next_vaddr(bytes, &vaddr);
    if (size < bytes)
        return -1;

    pages = ceil((double) PROC_INITIAL_STACK_SIZE / SIZE_4KB);
    paddr = pfa_request_pages(pages);
    if (paddr == 0)
        return -1;
    
    size = pml4_map_memory(ps->pml4, paddr, vaddr, bytes, PAGE_ACCESS_RW, PL0);
    if (size < bytes)
    {
        pfa_free_pages(paddr, pages);
        return -1;
    }
    
    kernel_stack_pages = malloc(sizeof(pages_list_entry_t));
    if (kernel_stack_pages == NULL)
    {
        pfa_free_pages(paddr, pages);
        return -1;
    }
    
    kernel_stack_pages->paddr = paddr;
    kernel_stack_pages->pages = pages;
    kernel_stack_pages->next = NULL;

    ps->kernel_stack_pages.head = kernel_stack_pages;
    ps->kernel_stack_pages.tail = kernel_stack_pages;
    ps->kernel_stack_start_vaddr = vaddr;

    return 0;
}

static char** process_parse_args(char* c, uint64_t* argc, uint64_t* bytes)
{
    char** argv;
    uint64_t i;

    i = 0;
    *argc = 1;
    *bytes = strlen(c);

#define SINGLE_QUOTE_OPEN_BIT (1 << 0)
#define DOUBLE_QUOTE_OPEN_BIT (1 << 1)

    while (*c != '\0')
    {
        switch (*c)
        {
        case '"':
            i ^= DOUBLE_QUOTE_OPEN_BIT;
            break;
        case '\'':
            i ^= SINGLE_QUOTE_OPEN_BIT;
            break;
        case ' ':
            if ((i & (DOUBLE_QUOTE_OPEN_BIT | SINGLE_QUOTE_OPEN_BIT)))
                break;
            *c = '\0';
            *argc += 1;
            break;
        }
        ++c;
    }

    argv = calloc(1, *argc * sizeof(char*));
    
    for 
    (
        c = c - *bytes,
        i = 0;
        i < *argc;
        c++
    )
    {
        if (argv[i] == NULL)
            argv[i] = c;
        if (*c == '\0')
        {
            if (*(c - 1) == '\'' || *(c - 1) == '"')
            {
                ++argv[i];
                *(c - 1) = '\0';
            }
            ++i;
        }
    }

    return argv;
}

static int process_load_args(process_t* ps, const char* cmdline)
{
    pages_list_entry_t* args_pages;
    uint64_t argc;
    uint64_t cmdline_bytes, argv_bytes, total_bytes, pages, size;
    uint64_t paddr, vaddr, tmp_vaddr;
    char** argv;

    if (cmdline == NULL)
        return 0;
    
    argv = process_parse_args((char*) cmdline, &argc, &cmdline_bytes);
    argv_bytes = argc * sizeof(char*);

    if (cmdline_bytes > SIZE_4KB)
    {
        free(argv);
        return -1;
    }

    total_bytes = cmdline_bytes + argv_bytes;
    size = pml4_get_next_vaddr(ps->pml4, ps->code_start_vaddr, total_bytes, &vaddr);
    if (size < total_bytes)
    {
        free(argv);
        return -1;
    }
    
    pages = ceil((double) total_bytes / SIZE_4KB);
    paddr = pfa_request_pages(pages);
    if (paddr == 0)
    {
        free(argv);
        return -1;
    }

    tmp_vaddr = kernel_map_temporary_page(paddr, PAGE_ACCESS_RW, PL0);
    memcpy((void*) tmp_vaddr, cmdline, cmdline_bytes);
    kernel_unmap_temporary_page(tmp_vaddr);

    tmp_vaddr = kernel_map_temporary_page(paddr + cmdline_bytes, PAGE_ACCESS_RW, PL0);
    memcpy((void*) tmp_vaddr, argv, argv_bytes);
    kernel_unmap_temporary_page(tmp_vaddr);

    free(argv);

    size = pml4_map_memory(ps->pml4, paddr, vaddr, total_bytes, PAGE_ACCESS_RW, PL3);
    if (size < total_bytes)
    {
        pfa_free_pages(paddr, pages);
        return -1;
    }

    args_pages = malloc(sizeof(pages_list_entry_t));
    if (args_pages == NULL)
    {
        pfa_free_pages(paddr, pages);
        return -1;
    }

    args_pages->paddr = paddr;
    args_pages->pages = pages;
    args_pages->next = NULL;

    ps->args_pages.head = args_pages;
    ps->args_pages.tail = args_pages;
    ps->args_start_vaddr = vaddr;

    ps->user_mode.regs.rdi = argc;
    ps->user_mode.regs.rsi = ps->args_start_vaddr;

    return 0;
}

static uint64_t process_delete_pages_list(pages_list_t* list)
{
    pages_list_entry_t* current;
    pages_list_entry_t* tmp;
    uint64_t pages;
    
    pages = 0;
    current = list->head;
    while (current != NULL)
    {
        pages += current->pages;
        tmp = current->next;
        pfa_free_pages(current->paddr, current->pages);
        free(current);
        current = tmp;
    }

    return pages * SIZE_4KB;
}

void process_delete_resources(process_t* ps)
{
    uint64_t i;

    if (ps->pml4 != 0)
    {
        if (pml4_delete(ps->pml4, ps->pml4_paddr) > KERNEL_HEAP_START_ADDR)
            return;
        kernel_unmap_memory((uint64_t) ps->pml4, SIZE_4KB);
    }
    
    if (ps->kernel_stack_start_vaddr != 0)
        process_delete_pages_list(&ps->kernel_stack_pages);
    if (ps->args_start_vaddr != 0)
        process_delete_pages_list(&ps->args_pages);

    process_delete_pages_list(&ps->code_pages);
    process_delete_pages_list(&ps->stack_pages);
    process_delete_pages_list(&ps->mapped_pages);

    for (i = 0; i < PROC_MAX_FDS; i++)
    {
        if (ps->fds[i] != 0)
        {
            /* Free file descriptors */
        }
    }
}

void process_delete_and_free(process_t* ps)
{
    process_delete_resources(ps);
    free(ps);
}

process_t* process_create(const char* path, uint64_t pid)
{
    process_t* ps;
    
    ps = malloc(sizeof(process_t));
    if (ps == NULL)
        return NULL;

    process_init(ps, pid);

    if 
    (
        process_init_pml4(ps) ||
        process_load_code(ps, path) || 
        process_init_stack(ps) || 
        process_init_kernel_stack(ps) ||
        process_load_args(ps, NULL) /* TODO: Add back CMDLINE support */
    )
    {
        process_delete_and_free(ps);
        return NULL;
    }

    ps->current = ps->user_mode;
    return ps;
}

static int copy_file_descriptors(file_descriptor_t* from, file_descriptor_t* to)
{
    uint64_t i;
    for (i = 0; i < PROC_MAX_FDS; i++)
    {
        if (from[i] != 0)
        {
            /* Implement file descriptor copy */
            to[i] = from[i];
        }
    }
    return 0;
}

process_t* process_create_replacement(const char* path, process_t* parent)
{
    process_t* child;
    
    child = process_create(path, parent->pid);
    if (child == NULL)
        return NULL;
    
    child->parent_pid = parent->parent_pid;
    if (copy_file_descriptors(parent->fds, child->fds))
    {
        process_delete_and_free(child);
        return NULL;
    }
    
    return child;
}

static int copy_segments_list(page_table_t pml4, uint64_t vaddr, pages_list_t* from, pages_list_t* to)
{
    uint64_t bytes, size, paddr, kernel_vaddr;
    pages_list_entry_t* segment;
    pages_list_entry_t* ptr;

    ptr = from->head;
    while (ptr != NULL)
    {
        paddr = pfa_request_pages(ptr->pages);
        if (paddr == 0)
            return -1;
        
        bytes = ptr->pages * SIZE_4KB;
        
        size = kernel_get_next_vaddr(bytes, &kernel_vaddr);
        if (size < bytes)
        {
            pfa_free_pages(paddr, ptr->pages);
            return -1;
        }
        
        size = kernel_map_memory(paddr, kernel_vaddr, bytes, PAGE_ACCESS_RW, PL0);
        if (size < bytes)
        {
            kernel_unmap_memory(kernel_vaddr, bytes);
            pfa_free_pages(paddr, ptr->pages);
            return -1;
        }
        
        memcpy((void*) kernel_vaddr, (void*) vaddr, bytes);
        kernel_unmap_memory(kernel_vaddr, bytes);
        
        segment = malloc(sizeof(pages_list_entry_t));
        if (segment == NULL)
        {
            pfa_free_pages(paddr, ptr->pages);
            return -1;
        }

        segment->paddr = paddr;
        segment->pages = ptr->pages;
        segment->next = NULL;

        if (to->head == NULL)
            to->head = segment;
        else
            to->tail->next = segment;
        to->tail = segment;

        size = pml4_map_memory(pml4, paddr, vaddr, bytes, PAGE_ACCESS_WX, PL3);
        if (size < bytes)
        {
            free(segment);
            pfa_free_pages(paddr, ptr->pages);
            return -1;
        }

        vaddr += bytes;
        ptr = ptr->next;
    }

    return 0;
}

process_t* process_clone(process_t* parent, uint64_t id)
{
    process_t* child;
    
    child = malloc(sizeof(process_t));
    if (child == NULL)
        return NULL;
    
    process_init(child, id);

    child->parent_pid = parent->pid;
    child->user_mode = parent->user_mode;

    child->code_start_vaddr = parent->code_start_vaddr;
    child->stack_start_vaddr = parent->stack_start_vaddr;
    child->mapping_start_vaddr = parent->mapping_start_vaddr;
    child->args_start_vaddr = parent->args_start_vaddr;

    if 
    (
        process_init_pml4(child) ||
        copy_segments_list(child->pml4, parent->code_start_vaddr, &parent->code_pages, &child->code_pages) ||
        copy_segments_list(child->pml4, parent->stack_start_vaddr, &parent->stack_pages, &child->stack_pages) ||
        copy_segments_list(child->pml4, parent->mapping_start_vaddr, &parent->mapped_pages, &child->mapped_pages) ||
        copy_segments_list(child->pml4, parent->args_start_vaddr, &parent->args_pages, &child->args_pages) ||
        process_init_kernel_stack(child) || 
        copy_file_descriptors(parent->fds, child->fds)
    )
    {
        process_delete_and_free(child);
        return NULL;
    }

    return child;
}