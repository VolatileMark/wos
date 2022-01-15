#ifndef __PROCESS_H__
#define __PROCESS_H__

#include "../cpu/isr.h"
#include "../mem/paging.h"
#include "../mem/heap.h"
#include <stdint.h>

#define PROCESS_MAX_FDS 64

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
    uint64_t start_vaddr;
    uint64_t end_vaddr;
    uint64_t ceil_vaddr;
    heap_segment_header_t* head;
    heap_segment_header_t* tail;
    segment_list_t segments;
} process_heap_t;

typedef struct process
{
    uint64_t pid;
    uint64_t parent_pid;
    page_table_t pml4;
    uint64_t pml4_paddr;
    cpu_state_t current;
    cpu_state_t user_mode;
    uint64_t kernel_stack_start_vaddr;
    uint64_t stack_start_vaddr;
    uint64_t code_start_vaddr;
    file_descriptor_t fds[PROCESS_MAX_FDS];
    segment_list_t code_segments;
    segment_list_t stack_segments;
    segment_list_t kernel_stack_segments;
    process_heap_t heap;
} process_t;

typedef struct 
{
    PROC_EXEC_TYPE type;
    uint64_t paddr;
    uint64_t size;
} process_file_t;

process_t* create_process(const process_file_t* file, uint64_t pid);
process_t* create_replacement_process(process_t* parent, const process_file_t* file);
process_t* clone_process(process_t* parent, uint64_t id);
void delete_and_free_process(process_t* ps);
void delete_process_resources(process_t* ps);
void load_process_pml4(process_t* ps);

#endif