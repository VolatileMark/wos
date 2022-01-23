#if !defined(__HEAP_H__) && !defined(LIBC_UTILS_ONLY)
#define __HEAP_H__

#include "../kernel/utils/constants.h"
#include "stdint.h"

struct heap_segment_header 
{
    struct heap_segment_header* next;
    struct heap_segment_header* prev;
    uint64_t data;
    uint64_t size : 63;
    uint64_t free : 1;
} __attribute__((packed));
typedef struct heap_segment_header heap_segment_header_t;

struct heap;

typedef uint64_t (*expand_heap_func_t)(struct heap* heap, uint64_t size);

typedef struct heap
{
    uint64_t start_vaddr;
    uint64_t end_vaddr;
    uint64_t ceil_vaddr;
    heap_segment_header_t* head;
    heap_segment_header_t* tail;
    PRIVILEGE_LEVEL privilege_level;
    expand_heap_func_t expand;
} heap_t;

heap_t* get_heap(void);
int init_heap(expand_heap_func_t expand_heap_func, uint64_t start_vaddr, uint64_t ceil_vaddr, uint64_t initial_size, PRIVILEGE_LEVEL privilege_level);
uint64_t allocate_heap_memory(uint64_t size);
void free_heap_memory(uint64_t addr);

#endif