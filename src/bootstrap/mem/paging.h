#ifndef __PAGING_H__
#define __PAGING_H__

#include <stdint.h>
#include "../utils/constants.h"

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

struct page_table
{
    page_table_entry_t entries[512];
} __attribute__((aligned(SIZE_4KB)));
typedef struct page_table page_table_t;

struct address_indices
{
    uint16_t pml4;
    uint16_t pdp;
    uint16_t pd;
    uint16_t pt;
};
typedef struct address_indices address_indices_t;

#define CLEAR_PTE(pte) *((uint64_t*) pte) = 0
//#define GET_PTE_ADDRESS(pte) (*((uint64_t*) pte) & 0x000FFFFFFFFFF000)
//#define SET_PTE_ADDRESS(pte, addr) *((uint64_t*) pte) = ((*((uint64_t*) pte) & 0xFFF0000000000FFF) | (addr & 0x000FFFFFFFFFF000))
#define I2A(pml4, pdp, pd, pt) ((((uint64_t) ((pml4 < 480) ? 0x0000 : 0xFFFF)) << 48) | ((((uint64_t) pml4) << 39) | (((uint64_t) pdp) << 30) | (((uint64_t) pd) << 21) | (((uint64_t) pt) << 12)))
#define A2I(addr) address_indices_t indices = { .pml4 = (addr >> 39) & 0x01FF, .pdp = (addr >> 30) & 0x01FF, .pd = (addr >> 21) & 0x01FF, .pt = (addr >> 12) & 0x01FF }
#define IS_TMP_VADDR(indices) ((indices)->pml4 == 511 && (indices)->pdp == 510 && (indices)->pd == 0 && (indices)->pt < 512 && (indices)->pt >= 2)

void paging_init(void);
uint64_t paging_map_memory(page_table_t* pml4, uint64_t paddr, uint64_t vaddr, uint64_t size, uint8_t allow_writes, uint8_t allow_user_access);
uint64_t paging_default_map_memory(uint64_t paddr, uint64_t vaddr, uint64_t size);
uint64_t paging_unmap_memory(page_table_t* pml4, uint64_t vaddr, uint64_t size);
uint64_t paging_default_unmap_memory(uint64_t vaddr, uint64_t size);

#endif