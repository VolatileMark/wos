#include "process.h"
#include "../headers/elf.h"
#include "../utils/alloc.h"
#include "../utils/log.h"
#include "../sys/mem/pfa.h"
#include "../sys/cpu/gdt.h"
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <mem.h>

#define trace_process(msg, ...) trace("PROC", msg, ##__VA_ARGS__)

static void process_release_all_memory(process_t* ps)
{
    memory_segments_list_entry_t* entry;
    for (entry = ps->mem.head; entry != NULL; entry = entry->next)
        pfa_free_pages(entry->paddr, entry->pages);
}

void process_delete_resources(process_t* ps)
{
    process_release_all_memory(ps);
    if (ps->exec_path != NULL)
        free((void*) ps->exec_path);
    if (ps->pml4 != NULL)
        pml4_delete(ps->pml4, ps->pml4_paddr);
}

void process_delete_and_free(process_t* ps)
{
    process_delete_resources(ps);
    free(ps);
}

static int process_request_memory(process_t* ps, uint64_t size, uint64_t hint, page_access_type_t access, privilege_level_t privilege, uint64_t* vaddr_out, uint64_t* paddr_out)
{
    uint64_t vaddr, paddr, pages;
    memory_segments_list_entry_t* entry;
    
    if (size == 0)
        return 0;

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
        pml4_map_memory(ps->pml4, paddr, vaddr, size, access, privilege) < size
    )
    {
        free(entry);
        pml4_unmap_memory(ps->pml4, vaddr, size);
        return -1;
    }

    entry->next = NULL;
    entry->paddr = paddr;
    entry->pages = pages;

    if (ps->mem.tail == NULL)
        ps->mem.head = entry;
    else
        ps->mem.tail->next = entry;
    ps->mem.tail = entry;
    
    if (vaddr_out != NULL)
        *vaddr_out = vaddr;
    if (paddr_out != NULL)
        *paddr_out = paddr;

    return 0;
}

static int process_create_pml4(process_t* ps)
{
    ps->pml4 = aligned_alloc(SIZE_4KB, SIZE_4KB);
    if (ps->pml4 == NULL)
        return -1;
    ps->pml4_paddr = kernel_get_paddr((uint64_t) ps->pml4);
    return 0;
}

static int process_load_exec_path(process_t* ps, const char* path)
{
    ps->exec_path = malloc(strlen(path));
    if (ps->exec_path == NULL)
        return -1;
    strcpy((char*) ps->exec_path, path);
    return 0;
}

static int process_load_elf(process_t* ps, const char* path)
{
    vnode_t file;
    vattribs_t attr;
    uint8_t* buffer;
    char* interp_path;
    uint64_t phdrs_offset, phdrs_total_size;
    uint64_t paddr, tmp_vaddr;
    Elf64_Ehdr* ehdr;
    Elf64_Phdr* phdr;

    if 
    (
        vfs_lookup(path, &file) ||
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

    phdrs_offset = (((uint64_t) ehdr) + ehdr->e_phoff);
    phdrs_total_size = ehdr->e_phentsize * ehdr->e_phnum;
    interp_path = NULL;

    for 
    (
        phdr = (Elf64_Phdr*) phdrs_offset;
        phdr < (Elf64_Phdr*) (((uint64_t) phdrs_offset) + phdrs_total_size);
        phdr = (Elf64_Phdr*) (((uint64_t) phdr) + ehdr->e_phentsize)
    )
    {
        if (phdr->p_vaddr + phdr->p_memsz > ps->brk_vaddr)
            ps->brk_vaddr = phdr->p_vaddr + phdr->p_memsz;

        switch (phdr->p_type)
        {
        case PT_INTERP:
            if 
            (
                interp_path != NULL ||
                ps->mem.head != NULL
            )
                return -1;
            interp_path = malloc(phdr->p_memsz);
            strcpy(interp_path, (char*) (buffer + phdr->p_offset));
            free(buffer);
            process_load_elf(ps, interp_path);
            free(interp_path);
            return 0;
            
        case PT_LOAD:
            if (process_request_memory(ps, phdr->p_memsz, phdr->p_vaddr, PAGE_ACCESS_RO, PL3, NULL, &paddr))
            {
                free(buffer);
                return -1;
            }
            if 
            (
                kernel_get_next_vaddr(phdr->p_filesz, &tmp_vaddr) < phdr->p_filesz ||
                kernel_map_memory(paddr, tmp_vaddr, phdr->p_filesz, PAGE_ACCESS_RW, PL0) < phdr->p_filesz
            )
                return -1;
            memcpy((void*) tmp_vaddr, buffer + phdr->p_offset, phdr->p_filesz);
            kernel_unmap_memory(tmp_vaddr, phdr->p_filesz);
            break;
        }
    }

    ps->cpu.stack.rip = ehdr->e_entry;
    free(buffer);
    
    return 0;
}

static int process_build_user_stack(process_t* ps, const char** argv, const char** envp)
{
    int argc, envc, i;
    uint64_t argv_size, envp_size, total_args_size, cpysize;
    uint64_t args_kvaddr, stack_kvaddr;
    uint64_t args_vaddr, args_paddr, stack_vaddr, stack_paddr;
    const char** stack_ptr;
    char* cpyptr;

    for (argv_size = 0, argc = 0; argv != NULL && argv[argc] != NULL; argc++)
        argv_size += strlen(argv[argc]) + 1;
    for (envp_size = 0, envc = 0; envp != NULL && envp[envc] != NULL; envc++)
        envp_size += strlen(envp[envc]) + 1;
    total_args_size = argv_size + envp_size;
    
    args_vaddr = alignd(PROC_CEIL_VADDR - total_args_size, SIZE_4KB);
    stack_vaddr = args_vaddr - PROC_USER_STACK_SIZE;
    if 
    (
        process_request_memory(ps, PROC_USER_STACK_SIZE, stack_vaddr, PAGE_ACCESS_RW, PL3, &stack_vaddr, &stack_paddr) ||
        process_request_memory(ps, total_args_size, args_vaddr, PAGE_ACCESS_RO, PL3, &args_vaddr, &args_paddr)
    )
        return -1;
    
    if 
    (
        kernel_get_next_vaddr(total_args_size, &args_kvaddr) < total_args_size ||
        kernel_map_memory(args_paddr, args_kvaddr, total_args_size, PAGE_ACCESS_RW, PL0) < total_args_size
    )
        return -1;
    if
    (
        kernel_get_next_vaddr(PROC_USER_STACK_SIZE, &stack_kvaddr) < PROC_USER_STACK_SIZE ||
        kernel_map_memory(stack_paddr, stack_kvaddr, PROC_USER_STACK_SIZE, PAGE_ACCESS_RW, PL0) < PROC_USER_STACK_SIZE
    )
        return -1;

    cpyptr = (char*) args_kvaddr;
    stack_ptr = (const char**) (stack_kvaddr + PROC_USER_STACK_SIZE - sizeof(uint64_t));
    
    stack_ptr = &stack_ptr[-envc];
    for (i = 0; envp != NULL && i < envc; i++)
    {
        cpysize = strlen(envp[i]) + 1;
        memcpy(cpyptr, envp[i], cpysize);
        stack_ptr[i] = (const char*) ((cpyptr - args_kvaddr) + args_vaddr);
        cpyptr += cpysize;
    }
    stack_ptr[envc] = NULL; 

    stack_ptr = &stack_ptr[-(argc + 1)];
    for (i = 0; argv != NULL && i < argc; i++)
    {
        cpysize = strlen(argv[i]) + 1;
        memcpy(cpyptr, argv[i], cpysize);
        stack_ptr[i] = (const char*) ((cpyptr - args_kvaddr) + args_vaddr);
        cpyptr += cpysize;
    }
    stack_ptr[argc] = NULL;

    kernel_unmap_memory(args_kvaddr, total_args_size);
    kernel_unmap_memory(stack_kvaddr, PROC_USER_STACK_SIZE);

    ps->user_stack_vaddr = stack_vaddr;
    ps->cpu.regs.rbp = ps->user_stack_vaddr + PROC_USER_STACK_SIZE - sizeof(uint64_t);
    ps->envp = &((const char**) ps->cpu.regs.rbp)[-envc];
    ps->argv = &ps->envp[-(argc + 1)];
    ps->cpu.stack.rsp = ((uint64_t) ps->argv) - sizeof(uint64_t);
    ps->cpu.regs.rdi = argc;
    ps->cpu.regs.rsi = (uint64_t) ps->argv;
    ps->cpu.regs.rdx = (uint64_t) ps->envp;

    return 0;
}

static int process_build_kernel_stack(process_t* ps)
{
    uint64_t hint, paddr;
    if 
    (
        kernel_get_next_vaddr(PROC_KERNEL_STACK_SIZE, &hint) < PROC_KERNEL_STACK_SIZE ||
        process_request_memory(ps, PROC_KERNEL_STACK_SIZE, hint, PAGE_ACCESS_RW, PL0, &ps->kernel_stack_vaddr, &paddr)
    )
        return -1;
    if (kernel_map_memory(paddr, hint, PROC_KERNEL_STACK_SIZE, PAGE_ACCESS_RW, PL0) < PROC_KERNEL_STACK_SIZE)
    {
        kernel_unmap_memory(hint, PROC_KERNEL_STACK_SIZE);
        return -1;
    }
    memset((void*) hint, 0, PROC_KERNEL_STACK_SIZE);
    kernel_unmap_memory(hint, PROC_KERNEL_STACK_SIZE);
    return 0;
}

process_t* process_create(const char* path, const char** argv, const char** envp, uint64_t pid)
{
    process_t* ps = calloc(1, sizeof(process_t));
    if (ps == NULL)
        return NULL;

    ps->pid = pid;
    ps->cpu.stack.cs = gdt_get_user_cs() | PL3;
    ps->cpu.stack.ss = gdt_get_user_ds() | PL3;
    ps->cpu.stack.rflags = PROC_DEFAULT_RFLAGS;
    
    if (process_create_pml4(ps))
    {
        trace_process("Failed to create process PML4 (pid: %u)", pid);
        process_delete_and_free(ps);
        return NULL;
    }

    if (process_load_elf(ps, path))
    {
        trace_process("Failed to load process executable (pid: %u)", pid);
        process_delete_and_free(ps);
        return NULL;
    }
    ps->brk_vaddr = alignu(ps->brk_vaddr, SIZE_1MB);

    if (process_build_user_stack(ps, argv, envp))
    {
        trace_process("Failed to build user stack (pid: %u)", pid);
        process_delete_and_free(ps);
        return NULL;
    }

    if (process_build_kernel_stack(ps))
    {
        trace_process("Failed to build kernel stack (pid: %u)", pid);
        process_delete_and_free(ps);
        return NULL;
    }

    if (process_load_exec_path(ps, path))
    {
        trace_process("Failed to set process executable path");
        process_delete_and_free(ps);
        return NULL;
    }

    return ps;
}