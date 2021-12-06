#include "paging.h"
#include "pfa.h"
#include <math.h>
#include <mem.h>

static page_table_t current_pml4;
static page_table_t kernel_tmp_pt;
static uint64_t kernel_tmp_index;

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

static uint64_t get_next_tmp_index(void)
{
    if (kernel_tmp_index >= MAX_TABLE_ENTRIES)
        return 0;
    uint64_t index = kernel_tmp_index++;
    for 
    (
        ; 
        kernel_tmp_pt[kernel_tmp_index].present; 
        kernel_tmp_index++
    );
    return index;
}

static void create_pte(page_table_t table, uint64_t index, uint64_t paddr, uint8_t allow_writes, uint8_t allow_user_access)
{
    page_table_entry_t* entry = &table[index];
    PTE_CLEAR(entry);
    set_pte_address(entry, paddr);
    entry->present = 1;
    entry->allow_writes = allow_writes;
    entry->allow_user_access = allow_user_access;
}

uint64_t paging_map_temporary_page(uint64_t paddr, uint8_t allow_writes, uint8_t allow_user_access)
{
    uint64_t index = get_next_tmp_index();
    page_table_entry_t* entry = &kernel_tmp_pt[index];
    PTE_CLEAR(entry);
    entry->present = 1;
    entry->allow_writes = 1;
    entry->allow_user_access = 1;
    set_pte_address(entry, paddr);
    return VADDR_GET_TEMPORARY(index);
}

void paging_unmap_temporary_page(uint64_t vaddr)
{
    uint64_t index;
    if (!VADDR_IS_TEMPORARY(vaddr))
        return;
    index = VADDR_TO_PT_IDX(vaddr);
    if (kernel_tmp_index > index)
        kernel_tmp_index = index;
    PTE_CLEAR(&kernel_tmp_pt[index]);
    invalidate_pte(vaddr);
}

void paging_init(void)
{
    current_pml4 = (page_table_t) PML4_VADDR;
    kernel_tmp_pt = (page_table_t) VADDR_GET_TEMPORARY(1);
    kernel_tmp_index = 2;
}

uint64_t paging_map_memory(uint64_t paddr, uint64_t vaddr, uint64_t size, uint8_t allow_writes, uint8_t allow_user_access)
{
    return pml4_map_memory(current_pml4, paddr, vaddr, size, allow_writes, allow_user_access);
}

uint64_t paging_unmap_memory(uint64_t vaddr, uint64_t size)
{
    return pml4_unmap_memory(current_pml4, vaddr, size);
}

static uint64_t pt_unmap_memory
(
    page_table_t pt, 
    uint64_t vaddr, 
    uint64_t size
)
{
    uint64_t pt_idx = VADDR_TO_PT_IDX(vaddr);
    uint64_t unmapped_size = 0;

    while (unmapped_size < size && pt_idx < MAX_TABLE_ENTRIES)
    {
        if (pt[pt_idx].present)
        {
            PTE_CLEAR(&pt[pt_idx]);
            invalidate_pte(vaddr);
        }

        vaddr += PT_ENTRY_SIZE;
        unmapped_size += PT_ENTRY_SIZE;
        ++pt_idx;
    }

    return unmapped_size;
}

static uint64_t pd_unmap_memory
(
    page_table_t pd, 
    uint64_t vaddr, 
    uint64_t size
)
{
    uint64_t pd_idx = VADDR_TO_PD_IDX(vaddr);
    page_table_t pt;
    page_table_entry_t entry;
    uint64_t i;
    uint64_t pt_paddr, pt_vaddr;
    uint64_t unmapped_size = 0, total_unmapped_size = 0;

    while (total_unmapped_size < size && pd_idx < MAX_TABLE_ENTRIES)
    {
        entry = pd[pd_idx];

        if (!entry.present)
        {
            vaddr = alignu(++vaddr, PD_ENTRY_SIZE);
            total_unmapped_size += PD_ENTRY_SIZE;
            ++pd_idx;
            continue;
        }

        pt_paddr = get_pte_address(&entry);
        pt_vaddr = paging_map_temporary_page(pt_paddr, 1, 1);

        pt = (page_table_t) pt_vaddr;
        unmapped_size = pt_unmap_memory(pt, vaddr, size - total_unmapped_size);

        for (i = 0; i < MAX_TABLE_ENTRIES && !pt[i].present; i++);

        paging_unmap_temporary_page(pt_vaddr);

        if (i == MAX_TABLE_ENTRIES)
        {
            free_page(pt_paddr);
            PTE_CLEAR(&pd[pd_idx]);
        }

        total_unmapped_size += unmapped_size;
        vaddr += unmapped_size;
        ++pd_idx;
    }
    
    return total_unmapped_size;
}

static uint64_t pdp_unmap_memory
(
    page_table_t pdp, 
    uint64_t vaddr, 
    uint64_t size
)
{
    uint64_t pdp_idx = VADDR_TO_PDP_IDX(vaddr);
    page_table_t pd;
    page_table_entry_t entry;
    uint64_t i;
    uint64_t pd_paddr, pd_vaddr;
    uint64_t unmapped_size = 0, total_unmapped_size = 0;

    while (total_unmapped_size < size && pdp_idx < MAX_TABLE_ENTRIES)
    {
        entry = pdp[pdp_idx];

        if (!entry.present)
        {
            vaddr = alignu(++vaddr, PDP_ENTRY_SIZE);
            total_unmapped_size += PDP_ENTRY_SIZE;
            ++pdp_idx;
            continue;
        }

        pd_paddr = get_pte_address(&entry);
        pd_vaddr = paging_map_temporary_page(pd_paddr, 1, 1);

        pd = (page_table_t) pd_vaddr;
        unmapped_size = pd_unmap_memory(pd, vaddr, size - total_unmapped_size);

        for (i = 0; i < MAX_TABLE_ENTRIES && !pd[i].present; i++);

        paging_unmap_temporary_page(pd_vaddr);

        if (i == MAX_TABLE_ENTRIES)
        {
            free_page(pd_paddr);
            PTE_CLEAR(&pdp[pdp_idx]);
        }

        total_unmapped_size += unmapped_size;
        vaddr += unmapped_size;
        ++pdp_idx;
    }
    
    return total_unmapped_size;
}

uint64_t pml4_unmap_memory
(
    page_table_t pml4, 
    uint64_t vaddr, 
    uint64_t size
)
{
    uint64_t pml4_idx = VADDR_TO_PML4_IDX(vaddr);
    page_table_t pdp;
    page_table_entry_t entry;
    uint64_t i;
    uint64_t pdp_paddr, pdp_vaddr;
    uint64_t unmapped_size = 0, total_unmapped_size = 0;
    size = alignu(size, PT_ENTRY_SIZE);

    while (total_unmapped_size < size && pml4_idx < MAX_TABLE_ENTRIES)
    {
        entry = pml4[pml4_idx];

        if (!entry.present)
        {
            vaddr = alignu(++vaddr, PML4_ENTRY_SIZE);
            total_unmapped_size += PML4_ENTRY_SIZE;
            ++pml4_idx;
            continue;
        }

        pdp_paddr = get_pte_address(&entry);
        pdp_vaddr = paging_map_temporary_page(pdp_paddr, 1, 1);

        pdp = (page_table_t) pdp_vaddr;
        unmapped_size = pdp_unmap_memory(pdp, vaddr, size - total_unmapped_size);

        for (i = 0; i < MAX_TABLE_ENTRIES && !pdp[i].present; i++);

        paging_unmap_temporary_page(pdp_vaddr);

        if (i == MAX_TABLE_ENTRIES)
        {
            free_page(pdp_paddr);
            PTE_CLEAR(&pml4[pml4_idx]);
        }

        total_unmapped_size += unmapped_size;
        vaddr += unmapped_size;
        ++pml4_idx;
    }
    
    return total_unmapped_size;
}

static uint64_t pt_map_memory
(
    page_table_t pt,
    uint64_t paddr,
    uint64_t vaddr,
    uint64_t size,
    uint8_t allow_writes,
    uint8_t allow_user_access
)
{
    uint64_t pt_idx = VADDR_TO_PT_IDX(vaddr);
    uint64_t mapped_size = 0;

    while (mapped_size < size && pt_idx < MAX_TABLE_ENTRIES)
    {
        if (pt[pt_idx].present)
            return mapped_size;
        create_pte(pt, pt_idx, paddr, 1, 1);

        paddr += PT_ENTRY_SIZE;
        mapped_size += PT_ENTRY_SIZE;
        ++pt_idx;
    }

    return mapped_size;
}

static uint64_t pd_map_memory
(
    page_table_t pd,
    uint64_t paddr,
    uint64_t vaddr,
    uint64_t size,
    uint8_t allow_writes,
    uint8_t allow_user_access
)
{
    uint64_t pd_idx = VADDR_TO_PD_IDX(vaddr);
    page_table_t pt;
    page_table_entry_t entry;
    uint64_t pt_paddr, pt_vaddr;
    uint64_t mapped_size = 0, total_mapped_size = 0;

    while (total_mapped_size < size && pd_idx < MAX_TABLE_ENTRIES)
    {
        entry = pd[pd_idx];

        if (!entry.present)
        {
            pt_paddr = request_page();
            if (pt_paddr == 0)
                return 0;
            pt_vaddr = paging_map_temporary_page(pt_paddr, 1, 1);
            memset((void*) pt_vaddr, 0, SIZE_4KB);
            create_pte(pd, pd_idx, pt_paddr, 1, 1);
        }
        else
        {
            pt_paddr = get_pte_address(&entry);
            pt_vaddr = paging_map_temporary_page(pt_paddr, 1, 1);
        }

        pt = (page_table_t) pt_vaddr;
        mapped_size = pt_map_memory(pt, paddr, vaddr, size - total_mapped_size, allow_writes, allow_user_access);

        paging_unmap_temporary_page(pt_vaddr);

        if (mapped_size == 0)
            return 0;
        
        total_mapped_size += mapped_size;
        vaddr += mapped_size;
        paddr += mapped_size;
        ++pd_idx;
    }

    return total_mapped_size;
}

static uint64_t pdp_map_memory
(
    page_table_t pdp,
    uint64_t paddr,
    uint64_t vaddr,
    uint64_t size,
    uint8_t allow_writes,
    uint8_t allow_user_access
)
{
    uint64_t pdp_idx = VADDR_TO_PDP_IDX(vaddr);
    page_table_t pd;
    page_table_entry_t entry;
    uint64_t pd_paddr, pd_vaddr;
    uint64_t mapped_size = 0, total_mapped_size = 0;

    while (total_mapped_size < size && pdp_idx < MAX_TABLE_ENTRIES)
    {
        entry = pdp[pdp_idx];

        if (!entry.present)
        {
            pd_paddr = request_page();
            if (pd_paddr == 0)
                return 0;
            pd_vaddr = paging_map_temporary_page(pd_paddr, 1, 1);
            memset((void*) pd_vaddr, 0, SIZE_4KB);
            create_pte(pdp, pdp_idx, pd_paddr, 1, 1);
        }
        else
        {
            pd_paddr = get_pte_address(&entry);
            pd_vaddr = paging_map_temporary_page(pd_paddr, 1, 1);
        }

        pd = (page_table_t) pd_vaddr;
        mapped_size = pd_map_memory(pd, paddr, vaddr, size - total_mapped_size, allow_writes, allow_user_access);

        paging_unmap_temporary_page(pd_vaddr);

        if (mapped_size == 0)
            return 0;
        
        total_mapped_size += mapped_size;
        vaddr += mapped_size;
        paddr += mapped_size;
        ++pdp_idx;
    }

    return total_mapped_size;
}

uint64_t pml4_map_memory
(
    page_table_t pml4, 
    uint64_t paddr, 
    uint64_t vaddr, 
    uint64_t size, 
    uint8_t allow_writes, 
    uint8_t allow_user_access
)
{
    uint64_t pml4_idx = VADDR_TO_PML4_IDX(vaddr);
    page_table_t pdp;
    page_table_entry_t entry;
    uint64_t pdp_paddr, pdp_vaddr;
    uint64_t mapped_size = 0, total_mapped_size = 0;
    size = alignu(size, PT_ENTRY_SIZE);

    while (total_mapped_size < size && pml4_idx < MAX_TABLE_ENTRIES)
    {
        entry = pml4[pml4_idx];

        if (!entry.present)
        {
            pdp_paddr = request_page();
            if (pdp_paddr == 0)
                return 0;
            pdp_vaddr = paging_map_temporary_page(pdp_paddr, 1, 1);
            memset((void*) pdp_vaddr, 0, SIZE_4KB);
            create_pte(pml4, pml4_idx, pdp_paddr, 1, 1);
        }
        else
        {
            pdp_paddr = get_pte_address(&entry);
            pdp_vaddr = paging_map_temporary_page(pdp_paddr, 1, 1);
        }

        pdp = (page_table_t) pdp_vaddr;
        mapped_size = pdp_map_memory(pdp, paddr, vaddr, size - total_mapped_size, allow_writes, allow_user_access);

        paging_unmap_temporary_page(pdp_vaddr);

        if (mapped_size == 0)
            return 0;

        total_mapped_size += mapped_size;
        vaddr += mapped_size;
        paddr += mapped_size;
        ++pml4_idx;
    }

    return total_mapped_size;
}