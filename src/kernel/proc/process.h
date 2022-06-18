#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "vfs/vfs.h"
#include "../sys/mem/paging.h"
#include "../sys/cpu/isr.h"
#include "../utils/macros.h"

#define PROC_MAX_FDS 64
#define PROC_DEFAULT_RFLAGS 0x202
#define PROC_CEIL_VADDR 0x800000000000
#define PROC_MIN_STACK_SIZE SIZE_nKB(8)
#define PROC_MAX_STACK_SIZE SIZE_nMB(2)

typedef struct memory_segments_list_entry
{
    struct memory_segments_list_entry* next;
    uint64_t paddr;
    uint64_t vaddr;
    uint64_t pages;
    page_access_type_t access;
    privilege_level_t pl;
} memory_segments_list_entry_t;

typedef struct
{
    memory_segments_list_entry_t* head;
    memory_segments_list_entry_t* tail;
} memory_segments_list_t;

struct cpu_state
{
    registers_state_t regs;
    stack_state_t stack;
    fpu_state_t fpu;
} __attribute__((packed));
typedef struct cpu_state cpu_state_t;

typedef struct
{
    uint64_t floor;
    uint64_t ceil;
    uint64_t size;
} stack_t;

typedef vnode_t file_descriptor_t;

typedef struct
{
    uint64_t pid;
    uint64_t parent_pid;
    uint64_t pml4_paddr;
    page_table_t pml4;
    const char* exec_path;
    const char** argv;
    const char** envp;
    uint64_t brk_vaddr;
    stack_t user_stack;
    stack_t kernel_stack;
    cpu_state_t cpu;
    memory_segments_list_t mem;
    file_descriptor_t fds[PROC_MAX_FDS];
} process_t;

process_t* process_create_replacement(process_t* parent, const char* path, const char** argv, const char** envp);
process_t* process_create(const char* path, const char** argv, const char** envp, uint64_t pid);
process_t* process_clone(process_t* parent, uint64_t pid);
void process_delete_resources(process_t* ps);
void process_delete_and_free(process_t* ps);
int process_grow_stack(process_t* ps, stack_t* stack, uint64_t size);

#endif
