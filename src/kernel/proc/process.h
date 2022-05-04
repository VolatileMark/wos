#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "vfs/vnode.h"
#include "../sys/mem/paging.h"
#include "../sys/cpu/isr.h"
#include <stdint.h>

#define PROC_MAX_FDS 64

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

struct cpu_state
{
    stack_state_t stack;
    registers_state_t regs;
    fpu_state_t fpu;
} __attribute__((packed));
typedef struct cpu_state cpu_state_t;

typedef vnode_t file_descriptor_t;

typedef struct
{
    uint64_t pid;
    uint64_t ppid;
    uint64_t pml4_paddr;
    const char* exec;
    page_table_t pml4;
    cpu_state_t cpu;
    memory_segments_list_t mem;
    file_descriptor_t fds[PROC_MAX_FDS];
} process_t;

process_t* process_create(const char *path, uint64_t pid);

#endif