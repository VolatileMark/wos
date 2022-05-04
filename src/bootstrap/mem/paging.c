#include "paging.h"
#include "pfa.h"
#include <math.h>
#include <mem.h>

static page_table_t bootstrap_pml4;
static page_table_t bootstrap_tmp_pt;
static uint64_t bootstrap_tmp_index;

void paging_init(void)
{
    bootstrap_pml4 = (page_table_t) PML4_VADDR;
    bootstrap_tmp_pt = (page_table_t) VADDR_GET_TEMPORARY(1);
    bootstrap_tmp_index = 2;
}

void pte_set_address(page_table_entry_t* entry, uint64_t addr)
{
    addr >>= 12;
    entry->addr_lo = addr & 0x000000000000000F;
    addr >>= 4;
    entry->addr_mid = addr & 0x00000000FFFFFFFF;
    addr >>= 32;
    entry->addr_hi = addr & 0x000000000000000F;
}

uint64_t pte_get_address(page_table_entry_t* entry)
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

static uint64_t paging_get_next_tmp_index(void)
{
    uint64_t index;
    if (bootstrap_tmp_index >= PT_MAX_ENTRIES) { return 0; }
    index = bootstrap_tmp_index++;
    for 
    (
        ; 
        bootstrap_tmp_pt[bootstrap_tmp_index].present; 
        bootstrap_tmp_index++
    );
    return index;
}

static void pte_create(page_table_t table, uint64_t index, uint64_t paddr, page_access_type_t access, privilege_level_t privilege_level)
{
    page_table_entry_t* entry;
    entry = &table[index];
    PTE_CLEAR(entry);
    pte_set_address(entry, paddr);
    entry->present = (((uint64_t) access) & 0b0100) >> 2;
    entry->allow_writes = (((uint64_t) access) & 0b0001);
    entry->allow_user_access = (privilege_level == PL3);
    /* entry->no_execute = (((uint64_t) access) & 0b0010) >> 1; */
}

uint64_t paging_map_temporary_page(uint64_t paddr, page_access_type_t access, privilege_level_t privilege_level)
{
    uint64_t index;
    index = paging_get_next_tmp_index();
    pte_create(bootstrap_tmp_pt, index, paddr, access, privilege_level);
    return VADDR_GET_TEMPORARY(index);
}

void paging_unmap_temporary_page(uint64_t vaddr)
{
    uint64_t index;
    if (!VADDR_IS_TEMPORARY(vaddr)) { return; }
    index = VADDR_TO_PT_IDX(vaddr);
    if (bootstrap_tmp_index > index) { bootstrap_tmp_index = index; }
    PTE_CLEAR(&bootstrap_tmp_pt[index]);
    pte_invalidate(vaddr);
}

uint64_t paging_map_memory(uint64_t paddr, uint64_t vaddr, uint64_t size, page_access_type_t access, privilege_level_t privilege_level)
{
    return pml4_map_memory(bootstrap_pml4, paddr, vaddr, size, access, privilege_level);
}

uint64_t paging_unmap_memory(uint64_t vaddr, uint64_t size)
{
    return pml4_unmap_memory(bootstrap_pml4, vaddr, size);
}

static uint64_t pt_unmap_memory
(
    page_table_t pt, 
    uint64_t vaddr, 
    uint64_t size
)
{
    uint64_t pt_idx, unmapped_size;

    pt_idx = VADDR_TO_PT_IDX(vaddr);
    unmapped_size = 0;

    while (unmapped_size < size && pt_idx < PT_MAX_ENTRIES)
    {
        if (pt[pt_idx].present)
        {
            PTE_CLEAR(&pt[pt_idx]);
            pte_invalidate(vaddr);
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
    page_table_t pt;
    page_table_entry_t entry;
    uint64_t i;
    uint64_t pd_idx;
    uint64_t pt_paddr, pt_vaddr;
    uint64_t unmapped_size, total_unmapped_size;

    pd_idx = VADDR_TO_PD_IDX(vaddr);
    unmapped_size = 0;
    total_unmapped_size = 0;

    while (total_unmapped_size < size && pd_idx < PT_MAX_ENTRIES)
    {
        entry = pd[pd_idx];

        if (!entry.present)
        {
            vaddr = alignu(++vaddr, PD_ENTRY_SIZE);
            total_unmapped_size += PD_ENTRY_SIZE;
            ++pd_idx;
            continue;
        }

        pt_paddr = pte_get_address(&entry);
        pt_vaddr = paging_map_temporary_page(pt_paddr, PAGE_ACCESS_RW, PL0);

        pt = (page_table_t) pt_vaddr;
        unmapped_size = pt_unmap_memory(pt, vaddr, size - total_unmapped_size);

        for (i = 0; i < PT_MAX_ENTRIES && !pt[i].present; i++);

        paging_unmap_temporary_page(pt_vaddr);

        if (i == PT_MAX_ENTRIES)
        {
            pfa_free_page(pt_paddr);
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
    page_table_t pd;
    page_table_entry_t entry;
    uint64_t i;
    uint64_t pdp_idx;
    uint64_t pd_paddr, pd_vaddr;
    uint64_t unmapped_size, total_unmapped_size;

    pdp_idx = VADDR_TO_PDP_IDX(vaddr);
    unmapped_size = 0;
    total_unmapped_size = 0;

    while (total_unmapped_size < size && pdp_idx < PT_MAX_ENTRIES)
    {
        entry = pdp[pdp_idx];

        if (!entry.present)
        {
            vaddr = alignu(++vaddr, PDP_ENTRY_SIZE);
            total_unmapped_size += PDP_ENTRY_SIZE;
            ++pdp_idx;
            continue;
        }

        pd_paddr = pte_get_address(&entry);
        pd_vaddr = paging_map_temporary_page(pd_paddr, PAGE_ACCESS_RW, PL0);

        pd = (page_table_t) pd_vaddr;
        unmapped_size = pd_unmap_memory(pd, vaddr, size - total_unmapped_size);

        for (i = 0; i < PT_MAX_ENTRIES && !pd[i].present; i++);

        paging_unmap_temporary_page(pd_vaddr);

        if (i == PT_MAX_ENTRIES)
        {
            pfa_free_page(pd_paddr);
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
    page_table_t pdp;
    page_table_entry_t entry;
    uint64_t i;
    uint64_t pml4_idx;
    uint64_t pdp_paddr, pdp_vaddr;
    uint64_t unmapped_size, total_unmapped_size;
    
    size = alignu(size + (vaddr - alignd(vaddr, SIZE_4KB)), PT_ENTRY_SIZE);
    pml4_idx = VADDR_TO_PML4_IDX(vaddr);
    unmapped_size = 0;
    total_unmapped_size = 0;

    while (total_unmapped_size < size && pml4_idx < PT_MAX_ENTRIES)
    {
        entry = pml4[pml4_idx];

        if (!entry.present)
        {
            vaddr = alignu(++vaddr, PML4_ENTRY_SIZE);
            total_unmapped_size += PML4_ENTRY_SIZE;
            ++pml4_idx;
            continue;
        }

        pdp_paddr = pte_get_address(&entry);
        pdp_vaddr = paging_map_temporary_page(pdp_paddr, PAGE_ACCESS_RW, PL0);

        pdp = (page_table_t) pdp_vaddr;
        unmapped_size = pdp_unmap_memory(pdp, vaddr, size - total_unmapped_size);

        for (i = 0; i < PT_MAX_ENTRIES && !pdp[i].present; i++);

        paging_unmap_temporary_page(pdp_vaddr);

        if (i == PT_MAX_ENTRIES)
        {
            pfa_free_page(pdp_paddr);
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
    page_access_type_t access,
    privilege_level_t privilege_level
)
{
    uint64_t pt_idx;
    uint64_t mapped_size;

    pt_idx = VADDR_TO_PT_IDX(vaddr);
    mapped_size = 0;

    while (mapped_size < size && pt_idx < PT_MAX_ENTRIES)
    {
        if (pt[pt_idx].present) { return 0; }
        pte_create(pt, pt_idx, paddr, access, privilege_level);

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
    page_access_type_t access,
    privilege_level_t privilege_level
)
{
    page_table_t pt;
    page_table_entry_t entry;
    uint64_t pd_idx;
    uint64_t pt_paddr, pt_vaddr;
    uint64_t mapped_size, total_mapped_size;

    pd_idx = VADDR_TO_PD_IDX(vaddr);
    mapped_size = 0;
    total_mapped_size = 0;

    while (total_mapped_size < size && pd_idx < PT_MAX_ENTRIES)
    {
        entry = pd[pd_idx];

        if (!entry.present)
        {
            pt_paddr = pfa_request_page();
            if (pt_paddr == 0) { return 0; }
            pt_vaddr = paging_map_temporary_page(pt_paddr, PAGE_ACCESS_RW, privilege_level);
            memset((void*) pt_vaddr, 0, SIZE_4KB);
            pte_create(pd, pd_idx, pt_paddr, PAGE_ACCESS_RW, privilege_level);
        }
        else
        {
            pt_paddr = pte_get_address(&entry);
            pt_vaddr = paging_map_temporary_page(pt_paddr, PAGE_ACCESS_RW, privilege_level);
        }

        pt = (page_table_t) pt_vaddr;
        mapped_size = pt_map_memory(pt, paddr, vaddr, size - total_mapped_size, access, privilege_level);

        paging_unmap_temporary_page(pt_vaddr);

        if (mapped_size == 0) { return 0; }
        
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
    page_access_type_t access,
    privilege_level_t privilege_level
)
{
    page_table_t pd;
    page_table_entry_t entry;
    uint64_t pdp_idx;
    uint64_t pd_paddr, pd_vaddr;
    uint64_t mapped_size, total_mapped_size;

    pdp_idx = VADDR_TO_PDP_IDX(vaddr);
    mapped_size = 0;
    total_mapped_size = 0;

    while (total_mapped_size < size && pdp_idx < PT_MAX_ENTRIES)
    {
        entry = pdp[pdp_idx];

        if (!entry.present)
        {
            pd_paddr = pfa_request_page();
            if (pd_paddr == 0) { return 0; }
            pd_vaddr = paging_map_temporary_page(pd_paddr, PAGE_ACCESS_RW, privilege_level);
            memset((void*) pd_vaddr, 0, SIZE_4KB);
            pte_create(pdp, pdp_idx, pd_paddr, PAGE_ACCESS_RW, privilege_level);
        }
        else
        {
            pd_paddr = pte_get_address(&entry);
            pd_vaddr = paging_map_temporary_page(pd_paddr, PAGE_ACCESS_RW, privilege_level);
        }

        pd = (page_table_t) pd_vaddr;
        mapped_size = pd_map_memory(pd, paddr, vaddr, size - total_mapped_size, access, privilege_level);

        paging_unmap_temporary_page(pd_vaddr);

        if (mapped_size == 0) { return 0; }
        
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
    page_access_type_t access, 
    privilege_level_t privilege_level
)
{
    page_table_t pdp;
    page_table_entry_t entry;
    uint64_t pml4_idx;
    uint64_t pdp_paddr, pdp_vaddr;
    uint64_t mapped_size, total_mapped_size;
    
    size = alignu(size + (paddr - alignd(paddr, SIZE_4KB)), PT_ENTRY_SIZE);
    pml4_idx = VADDR_TO_PML4_IDX(vaddr);
    mapped_size = 0;
    total_mapped_size = 0;

    while (total_mapped_size < size && pml4_idx < PT_MAX_ENTRIES)
    {
        entry = pml4[pml4_idx];

        if (!entry.present)
        {
            pdp_paddr = pfa_request_page();
            if (pdp_paddr == 0) { return 0; }
            pdp_vaddr = paging_map_temporary_page(pdp_paddr, PAGE_ACCESS_RW, privilege_level);
            memset((void*) pdp_vaddr, 0, SIZE_4KB);
            pte_create(pml4, pml4_idx, pdp_paddr, PAGE_ACCESS_RW, privilege_level);
        }
        else
        {
            pdp_paddr = pte_get_address(&entry);
            pdp_vaddr = paging_map_temporary_page(pdp_paddr, PAGE_ACCESS_RW, privilege_level);
        }

        pdp = (page_table_t) pdp_vaddr;
        mapped_size = pdp_map_memory(pdp, paddr, vaddr, size - total_mapped_size, access, privilege_level);

        paging_unmap_temporary_page(pdp_vaddr);

        if (mapped_size == 0) { return 0; }

        total_mapped_size += mapped_size;
        vaddr += mapped_size;
        paddr += mapped_size;
        ++pml4_idx;
    }

    return total_mapped_size;
}