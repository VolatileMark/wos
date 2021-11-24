#include "paging.h"
#include "pfa.h"
#include <math.h>
#include <mem.h>

static page_table_t* current_pml4;
static page_table_t* tmp_pt;
static uint64_t tmp_index;

extern page_table_t* get_current_pml4_paddr(void);

void set_pte_address(page_table_entry_t* entry, uint64_t addr)
{
    addr >>= 12;
    entry->addr_lo = addr & 0x000000000000000F;
    addr >>= 4;
    entry->addr_mid = addr & 0x00000000FFFFFFFF;
    addr >>= 32;
    entry->addr_hi = addr & 0x000000000000000F;
}

uint64_t get_pte_address(page_table_entry_t* entry)
{
    uint64_t addr;
    addr = entry->addr_hi;
    addr <<= 32;
    addr |= entry->addr_mid;
    addr <<= 4;
    addr |= entry->addr_lo;
    addr <<= 12;
    return addr;
}

void paging_init(void)
{
    current_pml4 = (page_table_t*) I2A(511, 510, 0, 0);
    tmp_pt = (page_table_t*) I2A(511, 510, 0, 1);
    tmp_index = 2;
}

uint64_t paging_map_temporary_address(uint64_t paddr)
{
    if (tmp_index >= 512)
        return 0;
    uint64_t index = tmp_index++;
    page_table_entry_t* entry;
    for (entry = &tmp_pt->entries[tmp_index]; tmp_index < 512 && entry->present; tmp_index++, entry = &tmp_pt->entries[tmp_index]);
    entry = &tmp_pt->entries[index];
    entry->present = 1;
    entry->allow_writes = 1;
    set_pte_address(entry, paddr);
    return I2A(511, 510, 0, index);
}

void paging_unmap_temporary_address(uint64_t vaddr)
{
    A2I(vaddr);
    if (!IS_TMP_VADDR(&indices))
        return;
    page_table_entry_t* entry = &tmp_pt->entries[indices.pt];
    CLEAR_PTE(entry);
    if (tmp_index > indices.pt)
        tmp_index = indices.pt;
}

uint64_t paging_default_map_memory(uint64_t paddr, uint64_t vaddr, uint64_t size)
{
    return paging_map_memory(current_pml4, paddr, vaddr, size, 1, 1);
}

uint64_t paging_default_unmap_memory(uint64_t vaddr, uint64_t size)
{
    return paging_unmap_memory(current_pml4, vaddr, size);
}

uint64_t paging_unmap_memory
(
    page_table_t* pml4, 
    uint64_t vaddr, 
    uint64_t size
)
{
    A2I(vaddr);
    uint64_t pages = ceil((double) size / SIZE_4KB);
    uint64_t unmapped_pages;
}

uint64_t paging_map_memory
(
    page_table_t* pml4, 
    uint64_t paddr, 
    uint64_t vaddr, 
    uint64_t size, 
    uint8_t allow_writes, 
    uint8_t allow_user_access
)
{
    A2I(vaddr);
    uint64_t pages = ceil((double) size / SIZE_4KB);
    uint64_t mapped_pages;
}