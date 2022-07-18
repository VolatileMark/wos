#ifndef __PAGING_H__
#define __PAGING_H__

#include <stdint.h>
#include "../../headers/constants.h"

struct page_table_entry
{
    uint8_t present : 1;
    uint8_t allow_writes : 1;
    uint8_t allow_user_access : 1;
    uint8_t write_through_enable : 1;
    uint8_t cache_disable : 1;
    uint8_t accessed : 1;
    uint8_t dirty : 1;
    uint8_t larger_pages : 1;
    uint8_t rsv0 : 4;
    uint8_t addr_lo : 4;
    uint32_t addr_mid;
    uint8_t addr_hi : 4;
    uint8_t rsv1 : 4;
    uint8_t rsv2 : 7;
    uint8_t no_execute : 1;
} __attribute__((packed));
typedef struct page_table_entry page_table_entry_t;

typedef page_table_entry_t* page_table_t;

typedef enum
{
    PAGE_ACCESS_RO = 0b0110,
    PAGE_ACCESS_RW = 0b0111
} page_access_type_t;

typedef enum
{
    PAGE_FLAG_PRESENT = 0,
    PAGE_FLAG_ALLOW_WRITES = 1,
    PAGE_FLAG_ALLOW_USER = 2,
    PAGE_FLAG_NO_WRITETHROUGH = 3,
    PAGE_FLAG_UNCACHABLE = 4,
    PAGE_FLAG_NO_EXECUTE = 63
} page_flag_t;

#define PTE_CLEAR(pte) *((uint64_t*) pte) = 0
#define VADDR_GET(pml4, pdp, pd, pt) ((((uint64_t) ((pml4 < 256) ? 0x0000 : 0xFFFF)) << 48) | ((((uint64_t) pml4) << 39) | (((uint64_t) pdp) << 30) | (((uint64_t) pd) << 21) | (((uint64_t) pt) << 12)))
#define VADDR_IS_TEMPORARY(addr) ((addr & 0xFFFFFFFFFFE00000) != 0xFFFF80000000)
#define VADDR_TO_PML4_IDX(addr) ((addr >> 39) & 0x1FF)
#define VADDR_TO_PDP_IDX(addr) ((addr >> 30) & 0x1FF)
#define VADDR_TO_PD_IDX(addr) ((addr >> 21) & 0x1FF)
#define VADDR_TO_PT_IDX(addr) ((addr >> 12) & 0x1FF)
#define VADDR_GET_TEMPORARY(idx) VADDR_GET(511, 510, 0, idx)
#define GET_ADDR_OFFSET(addr) (addr & 0x0000000000000FFF)

#define PT_MAX_ENTRIES 512
#define PT_ENTRY_SIZE ((uint64_t) SIZE_4KB)
#define PD_ENTRY_SIZE (PT_ENTRY_SIZE * PT_MAX_ENTRIES)
#define PDP_ENTRY_SIZE (PD_ENTRY_SIZE * PT_MAX_ENTRIES)
#define PML4_ENTRY_SIZE (PDP_ENTRY_SIZE * PT_MAX_ENTRIES)

extern void pml4_load(uint64_t pml4_paddr);
extern void tlb_flush(void);
extern void pte_invalidate(uint64_t vaddr);

uint64_t kernel_get_pml4_paddr(void);

page_table_t paging_get_current_pml4(void);
extern uint64_t paging_get_current_pml4_paddr(void);

void pte_set_address(page_table_entry_t* entry, uint64_t addr);
uint64_t pte_get_address(page_table_entry_t* entry);

void paging_init(void);

uint64_t pml4_map_memory(page_table_t pml4, uint64_t paddr, uint64_t vaddr, uint64_t size, page_access_type_t access, privilege_level_t privilege_level);
uint64_t paging_map_memory(uint64_t paddr, uint64_t vaddr, uint64_t size, page_access_type_t access, privilege_level_t privilege_level);
uint64_t paging_map_temporary_page(uint64_t paddr, page_access_type_t access, privilege_level_t privilege_level);

uint64_t pml4_unmap_memory(page_table_t pml4, uint64_t vaddr, uint64_t size);
uint64_t paging_unmap_memory(uint64_t vaddr, uint64_t size);
void paging_unmap_temporary_page(uint64_t vaddr);

uint64_t kernel_get_next_vaddr(uint64_t size, uint64_t* vaddr_out);
uint64_t paging_get_next_vaddr(uint64_t vaddr_start, uint64_t size, uint64_t* vaddr_out);
uint64_t pml4_get_next_vaddr(page_table_t pml4, uint64_t vaddr_start, uint64_t size, uint64_t* vaddr_out);

uint64_t paging_get_paddr(uint64_t vaddr);
uint64_t pml4_get_paddr(page_table_t pml4, uint64_t vaddr);

uint64_t pml4_delete(page_table_t pml4, uint64_t pml4_paddr);
int paging_inject_kernel_pml4(page_table_t pml4);

int paging_set_pte_flag(uint64_t vaddr, page_flag_t flag);
int pml4_set_pte_flag(page_table_t pml4, uint64_t vaddr, page_flag_t flag);

int paging_reset_pte_flag(uint64_t vaddr, page_flag_t flag);
int pml4_reset_pte_flag(page_table_t pml4, uint64_t vaddr, page_flag_t flag);

int paging_flag_memory_area(uint64_t vaddr, uint64_t size, page_flag_t flag);
int pml4_flag_memory_area(page_table_t pml4, uint64_t vaddr, uint64_t size, page_flag_t flag);

int paging_unflag_memory_area(uint64_t vaddr, uint64_t size, page_flag_t flag);
int pml4_unflag_memory_area(page_table_t pml4, uint64_t vaddr, uint64_t size, page_flag_t flag);

#endif
