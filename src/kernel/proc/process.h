#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "vfs/vnode.h"
#include "../sys/mem/paging.h"
#include "../sys/cpu/isr.h"
#include "../utils/macros.h"
#include <stdint.h>

#define PROC_MAX_FDS 64
#define PROC_DEFAULT_RFLAGS 0x202
#define PROC_CEIL_VADDR 0x800000000000
#define PROC_STACK_SIZE SIZE_nMB(2)

typedef struct memory_segments_list_entry
{
    struct memory_segments_list_entry* next;
    uint64_t paddr;
    uint64_t pages;
} memory_segments_list_entry_t;

typedef struct
{
    memory_segments_list_entry_t* head;
    memory_segments_list_entry_t* tail;
} memory_segments_list_t;

typedef struct
{
    __attribute__((aligned(16))) fpu_state_t fpu;
    stack_state_t stack;
    registers_state_t regs;
} cpu_state_t;

typedef vnode_t file_descriptor_t;

typedef struct
{
    uint64_t pid;
    uint64_t ppid;
    uint64_t pml4_paddr;
    page_table_t pml4;
    const char* exec_path;
    const char** argv;
    const char** envp;
    uint64_t brk_vaddr;
    uint64_t stack_vaddr;
    cpu_state_t cpu;
    memory_segments_list_t mem;
    file_descriptor_t fds[PROC_MAX_FDS];
} process_t;

process_t* process_create(const char* path, const char** argv, const char** envp, uint64_t pid);

#endif