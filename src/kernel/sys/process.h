#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "../cpu/isr.h"
#include "../mem/paging.h"
#include <stdint.h>

#define PROC_MAX_FDS 64
#define PROC_INITIAL_RSP (VADDR_GET_TEMPORARY(0) - sizeof(uint64_t))
#define PROC_INITIAL_STACK_SIZE SIZE_4KB
#define PROC_DEFAULT_STACK_VADDR (VADDR_GET_TEMPORARY(0) - SIZE_4KB)
#define PROC_DEFAULT_RFLAGS 0x202 /* Only interrupts enabled */
#define PROC_DEFAULT_HEAP_VADDR 0xFFFF800000000000

typedef enum
{
    PROC_EXEC_ELF,
    PROC_EXEC_BIN
} PROC_EXEC_TYPE;

typedef struct segment_list_entry
{
    uint64_t paddr;
    uint64_t pages;
    struct segment_list_entry* next;
} segment_list_entry_t;

typedef struct
{
    segment_list_entry_t* head;
    segment_list_entry_t* tail;
} segment_list_t;

typedef uint64_t file_descriptor_t;

typedef struct
{
    registers_state_t regs;
    stack_state_t stack;
    fpu_state_t fpu;
} cpu_state_t;

typedef struct
{
    uint64_t exit : 1;
    uint64_t exit_code : 8;
    uint64_t reserved : 55;
} process_flags_t;

typedef struct process
{
    uint64_t pid;
    uint64_t parent_pid;
    page_table_t pml4;
    uint64_t pml4_paddr;
    uint64_t kernel_stack_start_vaddr;
    uint64_t stack_start_vaddr;
    uint64_t code_start_vaddr;
    uint64_t heap_start_vaddr;
    uint64_t args_start_vaddr;
    process_flags_t flags;
    file_descriptor_t fds[PROC_MAX_FDS];
    segment_list_t code_segments;
    segment_list_t stack_segments;
    segment_list_t kernel_stack_segments;
    segment_list_t heap_segments;
    segment_list_t args_segments;
    cpu_state_t current;
    cpu_state_t user_mode;
} process_t;

typedef struct 
{
    PROC_EXEC_TYPE exec_type;
    uint64_t exec_paddr;
    uint64_t exec_size;
    char* cmdline;
} process_descriptor_t;

process_t* create_process(const process_descriptor_t* file, uint64_t pid);
process_t* create_replacement_process(process_t* parent, const process_descriptor_t* file);
process_t* clone_process(process_t* parent, uint64_t id);
void delete_and_free_process(process_t* ps);
void delete_process_resources(process_t* ps);
void load_process_pml4(process_t* ps);

#endif