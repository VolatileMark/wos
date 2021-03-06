#include "process.h"
#include "../headers/elf.h"
#include "../utils/alloc.h"
#include "../utils/log.h"
#include "../sys/mem/pfa.h"
#include "../sys/cpu/gdt.h"
#include "../kernel.h"
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
    if (ps->argv != NULL)
        free(ps->argv);
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
    if (paddr == 0)
    {
        free(entry);
        return -1;
    }

    if (pml4_map_memory(ps->pml4, paddr, vaddr, size, access, privilege) < size)
    {
        free(entry);
        pfa_free_pages(paddr, pages);
        return -1;
    }

    entry->next = NULL;
    entry->paddr = paddr;
    entry->vaddr = vaddr;
    entry->pages = pages;
    entry->access = access;
    entry->pl = privilege;

    if (ps->mem.tail == NULL)
        ps->mem.head = entry;
    else
        ps->mem.tail->next = entry;
    ps->mem.tail = entry;
    
    if (vaddr_out != NULL)
        *vaddr_out = vaddr;
    if (paddr_out != NULL)
        *paddr_out = paddr;
    
    for (; pages > 0; pages--)
    {
        vaddr = paging_map_temporary_page(paddr, PAGE_ACCESS_RW, PL0);
        memset((void*) vaddr, 0, SIZE_4KB);
        paging_unmap_temporary_page(vaddr);
        paddr += SIZE_4KB;
    }

    return 0;
}

static int process_create_pml4(process_t* ps)
{
    ps->pml4 = aligned_alloc(SIZE_4KB, SIZE_4KB);
    if (ps->pml4 == NULL)
        return -1;
    ps->pml4_paddr = paging_get_paddr((uint64_t) ps->pml4);
    memset(ps->pml4, 0, SIZE_4KB);
    return 0;
}

static int process_load_exec_path(process_t* ps, const char* path)
{
    ps->exec_path = malloc(strlen(path) + 1);
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
            if (process_request_memory(ps, phdr->p_memsz, phdr->p_vaddr, PAGE_ACCESS_RW, PL3, NULL, &paddr))
            {
                free(buffer);
                return -1;
            }
            if 
            (
                kernel_get_next_vaddr(phdr->p_filesz, &tmp_vaddr) < phdr->p_filesz ||
                paging_map_memory(paddr, tmp_vaddr, phdr->p_filesz, PAGE_ACCESS_RW, PL0) < phdr->p_filesz
            )
                return -1;
            memcpy((void*) tmp_vaddr, buffer + phdr->p_offset, phdr->p_filesz);
            paging_unmap_memory(tmp_vaddr, phdr->p_filesz);
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
    uint64_t total_args_size, cpysize;
    uint64_t args_kvaddr, stack_kvaddr;
    uint64_t args_vaddr;
    uint64_t args_paddr, stack_paddr;
    uint64_t args_pages;
    const char** stack_ptr;
    char* cpyptr;

    total_args_size = 0;
    for (argc = 0; argv != NULL && argv[argc] != NULL; argc++)
        total_args_size += strlen(argv[argc]) + 1;
    for (envc = 0; envp != NULL && envp[envc] != NULL; envc++)
        total_args_size += strlen(envp[envc]) + 1;
    
    ps->argv = calloc(1, (argc + envc + 2) * sizeof(const char*));
    if (ps->argv == NULL)
        return -1;
    ps->envp = &ps->argv[argc + 1];
    
    args_pages = ceil((double) total_args_size / SIZE_4KB);
    args_paddr = pfa_request_pages(args_pages);
    if 
    (
        kernel_get_next_vaddr(total_args_size, &args_kvaddr) < total_args_size ||
        paging_map_memory(args_paddr, args_kvaddr, total_args_size, PAGE_ACCESS_RW, PL0) < total_args_size
    )
    {
        pfa_free_pages(args_paddr, args_pages);
        return -1;
    }
    args_vaddr = alignd(PROC_CEIL_VADDR - total_args_size, SIZE_4KB);

    ps->user_stack.size = PROC_MIN_STACK_SIZE;
    ps->user_stack.floor = args_vaddr;
    ps->user_stack.ceil = ps->user_stack.floor - ps->user_stack.size;

    if 
    (
        process_request_memory(ps, ps->user_stack.size, ps->user_stack.ceil, PAGE_ACCESS_RW, PL3, &ps->user_stack.ceil, &stack_paddr) ||
        paging_get_next_vaddr(args_vaddr, total_args_size, &args_vaddr) < total_args_size ||
        pml4_map_memory(ps->pml4, args_paddr, args_vaddr, total_args_size, PAGE_ACCESS_RO, PL3) < total_args_size
    )
    {
        paging_unmap_memory(args_kvaddr, total_args_size);
        pfa_free_pages(args_paddr, args_pages);
        return -1;
    }
    
    if
    (
        kernel_get_next_vaddr(ps->user_stack.size, &stack_kvaddr) < ps->user_stack.size ||
        paging_map_memory(stack_paddr, stack_kvaddr, ps->user_stack.size, PAGE_ACCESS_RW, PL0) < ps->user_stack.size
    )
    {
        paging_unmap_memory(args_kvaddr, total_args_size);
        pfa_free_pages(args_paddr, args_pages);
        return -1;
    }

    cpyptr = (char*) args_kvaddr;
    stack_ptr = (const char**) (stack_kvaddr + ps->user_stack.size - sizeof(uint64_t));

    stack_ptr = &stack_ptr[-(envc + argc + 1)];
    for (i = 0; argv != NULL && i < argc; i++)
    {
        cpysize = strlen(argv[i]) + 1;
        memcpy(cpyptr, argv[i], cpysize);
        stack_ptr[i] = (const char*) ((cpyptr - args_kvaddr) + args_vaddr);
        ps->argv[i] = stack_ptr[i];
        cpyptr += cpysize;
    }

    stack_ptr = &stack_ptr[argc + 1];
    for (i = 0; envp != NULL && i < envc; i++)
    {
        cpysize = strlen(envp[i]) + 1;
        memcpy(cpyptr, envp[i], cpysize);
        stack_ptr[i] = (const char*) ((cpyptr - args_kvaddr) + args_vaddr);
        ps->envp[i] = stack_ptr[i];
        cpyptr += cpysize;
    }

    paging_unmap_memory(stack_kvaddr, PROC_MIN_STACK_SIZE);
    paging_unmap_memory(args_kvaddr, total_args_size);

    ps->cpu.regs.rbp = ps->user_stack.floor - sizeof(uint64_t);
    ps->cpu.regs.rdi = argc;
    ps->cpu.regs.rdx = (uint64_t) &((const char**) ps->cpu.regs.rbp)[-envc];
    ps->cpu.regs.rsi = (uint64_t) &((const char**) ps->cpu.regs.rdx)[-(argc + 1)];
    ps->cpu.stack.rsp = ps->cpu.regs.rsi - sizeof(uint64_t);

    return 0;
}

int process_grow_stack(process_t* ps, stack_t* stack, uint64_t size)
{
    uint64_t hint, vaddr;

    size = alignu(size, SIZE_4KB);
    hint = stack->ceil - size;
    if 
    (
        process_request_memory(ps, size, hint, PAGE_ACCESS_RW, PL3, &vaddr, NULL) ||
        vaddr != hint
    )
    {
        trace_process("Could not allocate memory for stack expansion (pid: %u)", ps->pid);
        return -1;
    }

    stack->ceil += size;
    stack->size += size;

    return 0;
}

static int process_build_kernel_stack(process_t* ps)
{
    ps->kernel_stack.floor = KERNEL_HEAP_START_ADDR;
    ps->kernel_stack.size = PROC_MIN_STACK_SIZE;
    ps->kernel_stack.ceil = ps->kernel_stack.floor - ps->kernel_stack.size;
    return process_request_memory(ps, ps->kernel_stack.size, ps->kernel_stack.ceil, PAGE_ACCESS_RW, PL0, &ps->kernel_stack.ceil, NULL);
}

process_t* process_create(const char* path, const char** argv, const char** envp, uint64_t pid)
{
    process_t* ps = calloc(1, sizeof(process_t));
    if (ps == NULL)
    {
        trace_process("Could not create process descriptor");
        return NULL;
    }

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

static void process_copy_file_descriptors(process_t* dest, process_t* src)
{
    int i;
    for (i = 0; i < PROC_MAX_FDS; i++)
        dest->fds[i] = src->fds[i];
}

process_t* process_create_replacement(process_t* parent, const char* path, const char** argv, const char** envp)
{
    process_t* child;

    child = process_create(path, argv, envp, parent->pid);
    if (child == NULL)
    {
        trace_process("Could not create replacement from process with id %u", parent->pid);
        return NULL;
    }

    child->parent_pid = parent->parent_pid;
    process_copy_file_descriptors(child, parent);

    return child;
}

static int process_copy_memory_mappings(process_t* child, process_t* parent)
{
    uint64_t tmp_vaddr, vaddr, paddr, bytes;
    memory_segments_list_entry_t* sentry;

    for (sentry = parent->mem.head; sentry != NULL; sentry = sentry->next)
    {
        bytes = sentry->pages * SIZE_4KB;
        if (kernel_get_next_vaddr(bytes, &tmp_vaddr) < bytes)
            return -1;
        if 
        (
            process_request_memory(child, bytes, sentry->vaddr, sentry->access, sentry->pl, &vaddr, &paddr) ||
            vaddr != sentry->vaddr
        )
            return -1;
        if (paging_map_memory(paddr, tmp_vaddr, bytes, PAGE_ACCESS_RW, PL0) < bytes)
            return -1;
        memcpy((void*) tmp_vaddr, (void*) sentry->vaddr, bytes);
        paging_unmap_memory(tmp_vaddr, bytes);
    }

    return 0;
}

process_t* process_clone(process_t* parent, uint64_t pid)
{
    process_t* child;
    uint64_t argc, envc, args_size;

    child = calloc(1, sizeof(process_t));
    if (child == NULL)
    {
        trace_process("Could not create process descriptor (pid: %u)", pid);
        return NULL;
    }

    child->pid = pid;
    child->parent_pid = parent->pid;
    child->cpu = parent->cpu;
    child->brk_vaddr = parent->brk_vaddr;
    child->user_stack = parent->user_stack;
    child->kernel_stack = parent->kernel_stack;

    for (envc = 0; parent->envp[envc] != NULL; envc++);
    argc = (((uint64_t) parent->envp) - ((uint64_t) parent->argv)) / sizeof(const char*);
    args_size = (argc + envc + 2) * sizeof(const char*);
    child->argv = calloc(1, args_size);
    child->envp = &child->argv[argc + 1];
    memcpy(child->argv, parent->argv, args_size);

    if (child->argv == NULL)
    {
        trace_process("Failed to copy argv and envp from parent process (pid: %u)", pid);
        process_delete_and_free(child);
        return NULL;
    }

    if (process_create_pml4(child))
    {
        trace_process("Failed to create process PML4 (pid: %u)", pid);
        process_delete_and_free(child);
        return NULL;
    }
    
    if (process_copy_memory_mappings(child, parent))
    {
        trace_process("Failed to copy parent's memory mappings (pid: %u)", pid);
        process_delete_and_free(child);
        return NULL;
    }

    process_copy_file_descriptors(child, parent);

    return child;
}
