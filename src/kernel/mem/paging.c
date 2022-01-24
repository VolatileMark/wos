#include "paging.h"
#include "pfa.h"
#include <math.h>
#include <mem.h>

#define KERNEL_PML4_VADDR VADDR_GET_TEMPORARY(0)

static page_table_t kernel_pml4;
static page_table_t kernel_tmp_pt;
static uint64_t kernel_tmp_index;
static uint64_t kernel_pml4_paddr;

uint64_t get_kernel_pml4_paddr(void)
{
    return kernel_pml4_paddr;
}

page_table_t get_kernel_pml4(void)
{
    return kernel_pml4;
}

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
    if (kernel_tmp_index >= MAX_PAGE_TABLE_ENTRIES)
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

static void create_pte(page_table_t table, uint64_t index, uint64_t paddr, PAGE_ACCESS_TYPE access, PRIVILEGE_LEVEL privilege_level)
{
    page_table_entry_t* entry = &table[index];
    PTE_CLEAR(entry);
    set_pte_address(entry, paddr);
    entry->present = 1;
    entry->allow_writes = access;
    entry->allow_user_access = (privilege_level > 0);
}

uint64_t kernel_map_temporary_page(uint64_t paddr, PAGE_ACCESS_TYPE access, PRIVILEGE_LEVEL privilege_level)
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

void kernel_unmap_temporary_page(uint64_t vaddr)
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

void init_paging(void)
{
    kernel_pml4 = (page_table_t) KERNEL_PML4_VADDR;
    kernel_tmp_pt = (page_table_t) VADDR_GET_TEMPORARY(1);
    kernel_tmp_index = 2;
    kernel_pml4_paddr = kernel_get_paddr(KERNEL_PML4_VADDR);
}

uint64_t kernel_map_memory(uint64_t paddr, uint64_t vaddr, uint64_t size, PAGE_ACCESS_TYPE access, PRIVILEGE_LEVEL privilege_level)
{
    return pml4_map_memory(kernel_pml4, paddr, vaddr, size, access, privilege_level);
}

uint64_t kernel_unmap_memory(uint64_t vaddr, uint64_t size)
{
    return pml4_unmap_memory(kernel_pml4, vaddr, size);
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

    while (unmapped_size < size && pt_idx < MAX_PAGE_TABLE_ENTRIES)
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

    while (total_unmapped_size < size && pd_idx < MAX_PAGE_TABLE_ENTRIES)
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
        pt_vaddr = kernel_map_temporary_page(pt_paddr, 1, 1);

        pt = (page_table_t) pt_vaddr;
        unmapped_size = pt_unmap_memory(pt, vaddr, size - total_unmapped_size);

        for (i = 0; i < MAX_PAGE_TABLE_ENTRIES && !pt[i].present; i++);

        kernel_unmap_temporary_page(pt_vaddr);

        if (i == MAX_PAGE_TABLE_ENTRIES)
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

    while (total_unmapped_size < size && pdp_idx < MAX_PAGE_TABLE_ENTRIES)
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
        pd_vaddr = kernel_map_temporary_page(pd_paddr, 1, 1);

        pd = (page_table_t) pd_vaddr;
        unmapped_size = pd_unmap_memory(pd, vaddr, size - total_unmapped_size);

        for (i = 0; i < MAX_PAGE_TABLE_ENTRIES && !pd[i].present; i++);

        kernel_unmap_temporary_page(pd_vaddr);

        if (i == MAX_PAGE_TABLE_ENTRIES)
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

    while (total_unmapped_size < size && pml4_idx < MAX_PAGE_TABLE_ENTRIES)
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
        pdp_vaddr = kernel_map_temporary_page(pdp_paddr, 1, 1);

        pdp = (page_table_t) pdp_vaddr;
        unmapped_size = pdp_unmap_memory(pdp, vaddr, size - total_unmapped_size);

        for (i = 0; i < MAX_PAGE_TABLE_ENTRIES && !pdp[i].present; i++);

        kernel_unmap_temporary_page(pdp_vaddr);

        if (i == MAX_PAGE_TABLE_ENTRIES)
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
    PAGE_ACCESS_TYPE access,
    PRIVILEGE_LEVEL privilege_level
)
{
    uint64_t pt_idx = VADDR_TO_PT_IDX(vaddr);
    uint64_t mapped_size = 0;

    while (mapped_size < size && pt_idx < MAX_PAGE_TABLE_ENTRIES)
    {
        if (pt[pt_idx].present)
            return 0;
        create_pte(pt, pt_idx, paddr, access, privilege_level);

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
    PAGE_ACCESS_TYPE access,
    PRIVILEGE_LEVEL privilege_level
)
{
    uint64_t pd_idx = VADDR_TO_PD_IDX(vaddr);
    page_table_t pt;
    page_table_entry_t entry;
    uint64_t pt_paddr, pt_vaddr;
    uint64_t mapped_size = 0, total_mapped_size = 0;

    while (total_mapped_size < size && pd_idx < MAX_PAGE_TABLE_ENTRIES)
    {
        entry = pd[pd_idx];

        if (!entry.present)
        {
            pt_paddr = request_page();
            if (pt_paddr == 0)
                return 0;
            pt_vaddr = kernel_map_temporary_page(pt_paddr, PAGE_ACCESS_RW, privilege_level);
            memset((void*) pt_vaddr, 0, SIZE_4KB);
            create_pte(pd, pd_idx, pt_paddr, 1, 1);
        }
        else
        {
            pt_paddr = get_pte_address(&entry);
            pt_vaddr = kernel_map_temporary_page(pt_paddr, PAGE_ACCESS_RW, privilege_level);
        }

        pt = (page_table_t) pt_vaddr;
        mapped_size = pt_map_memory(pt, paddr, vaddr, size - total_mapped_size, access, privilege_level);

        kernel_unmap_temporary_page(pt_vaddr);

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
    PAGE_ACCESS_TYPE access,
    PRIVILEGE_LEVEL privilege_level
)
{
    uint64_t pdp_idx = VADDR_TO_PDP_IDX(vaddr);
    page_table_t pd;
    page_table_entry_t entry;
    uint64_t pd_paddr, pd_vaddr;
    uint64_t mapped_size = 0, total_mapped_size = 0;

    while (total_mapped_size < size && pdp_idx < MAX_PAGE_TABLE_ENTRIES)
    {
        entry = pdp[pdp_idx];

        if (!entry.present)
        {
            pd_paddr = request_page();
            if (pd_paddr == 0)
                return 0;
            pd_vaddr = kernel_map_temporary_page(pd_paddr, PAGE_ACCESS_RW, privilege_level);
            memset((void*) pd_vaddr, 0, SIZE_4KB);
            create_pte(pdp, pdp_idx, pd_paddr, PAGE_ACCESS_RW, privilege_level);
        }
        else
        {
            pd_paddr = get_pte_address(&entry);
            pd_vaddr = kernel_map_temporary_page(pd_paddr, PAGE_ACCESS_RW, privilege_level);
        }

        pd = (page_table_t) pd_vaddr;
        mapped_size = pd_map_memory(pd, paddr, vaddr, size - total_mapped_size, access, privilege_level);

        kernel_unmap_temporary_page(pd_vaddr);

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
    PAGE_ACCESS_TYPE access, 
    PRIVILEGE_LEVEL privilege_level
)
{
    uint64_t pml4_idx = VADDR_TO_PML4_IDX(vaddr);
    page_table_t pdp;
    page_table_entry_t entry;
    uint64_t pdp_paddr, pdp_vaddr;
    uint64_t mapped_size = 0, total_mapped_size = 0;
    size = alignu(size, PT_ENTRY_SIZE);

    while (total_mapped_size < size && pml4_idx < MAX_PAGE_TABLE_ENTRIES)
    {
        entry = pml4[pml4_idx];

        if (!entry.present)
        {
            pdp_paddr = request_page();
            if (pdp_paddr == 0)
                return 0;
            pdp_vaddr = kernel_map_temporary_page(pdp_paddr, PAGE_ACCESS_RW, privilege_level);
            memset((void*) pdp_vaddr, 0, SIZE_4KB);
            create_pte(pml4, pml4_idx, pdp_paddr, PAGE_ACCESS_RW, privilege_level);
        }
        else
        {
            pdp_paddr = get_pte_address(&entry);
            pdp_vaddr = kernel_map_temporary_page(pdp_paddr, PAGE_ACCESS_RW, privilege_level);
        }

        pdp = (page_table_t) pdp_vaddr;
        mapped_size = pdp_map_memory(pdp, paddr, vaddr, size - total_mapped_size, access, privilege_level);

        kernel_unmap_temporary_page(pdp_vaddr);

        if (mapped_size == 0)
            return 0;

        total_mapped_size += mapped_size;
        vaddr += mapped_size;
        paddr += mapped_size;
        ++pml4_idx;
    }

    return total_mapped_size;
}

uint64_t kernel_get_next_vaddr(uint64_t size, uint64_t* vaddr_out)
{
    return pml4_get_next_vaddr(kernel_pml4, (uint64_t) &_end_vaddr, size, vaddr_out);
}

uint64_t pml4_get_next_vaddr(page_table_t pml4, uint64_t vaddr_start, uint64_t size, uint64_t* vaddr_out)
{
    uint64_t pml4_idx, pdp_idx, pd_idx, pt_idx;
    uint64_t total_size_found = 0;
    page_table_t pdp, pd, pt;
    page_table_entry_t entry;

    size = alignu(size, SIZE_4KB);
    vaddr_start = alignd(vaddr_start, PT_ENTRY_SIZE);

    pml4_idx = VADDR_TO_PML4_IDX(vaddr_start);
    pdp_idx = VADDR_TO_PDP_IDX(vaddr_start);
    pd_idx = VADDR_TO_PD_IDX(vaddr_start);
    pt_idx = VADDR_TO_PT_IDX(vaddr_start);

    for
    (
        ;
        total_size_found < size && pml4_idx < MAX_PAGE_TABLE_ENTRIES;
        pml4_idx++
    )
    {
        entry = pml4[pml4_idx];
        if (entry.present)
        {
            pdp = (page_table_t) kernel_map_temporary_page(get_pte_address(&entry), PAGE_ACCESS_RO, PL0);
            for
            (
                ;
                total_size_found < size && pdp_idx < MAX_PAGE_TABLE_ENTRIES;
                pdp_idx++
            )
            {
                entry = pdp[pdp_idx];
                if (entry.present)
                {
                    pd = (page_table_t) kernel_map_temporary_page(get_pte_address(&entry), PAGE_ACCESS_RO, PL0);
                    for
                    (
                        ;
                        total_size_found < size && pd_idx < MAX_PAGE_TABLE_ENTRIES;
                        pd_idx++
                    )
                    {
                        entry = pd[pd_idx];
                        if (entry.present)
                        {
                            pt = (page_table_t) kernel_map_temporary_page(get_pte_address(&entry), PAGE_ACCESS_RO, PL0);
                            for
                            (
                                ;
                                total_size_found < size && pt_idx < MAX_PAGE_TABLE_ENTRIES;
                                pt_idx++
                            )
                            {
                                entry = pt[pt_idx];
                                if (entry.present)
                                    total_size_found = 0;
                                else
                                    total_size_found += SIZE_4KB;
                            }
                            kernel_unmap_temporary_page((uint64_t) pt);
                            if (total_size_found < size)
                                pt_idx = 0;
                            else
                                --pt_idx;
                        }
                        else
                            total_size_found += PD_ENTRY_SIZE;
                    }
                    kernel_unmap_temporary_page((uint64_t) pd);
                    if (total_size_found < size)
                        pd_idx = 0;
                    else
                        --pd_idx;
                }
                else
                    total_size_found += PDP_ENTRY_SIZE;
            }
            kernel_unmap_temporary_page((uint64_t) pdp);
            if (total_size_found < size)
                pdp_idx = 0;
            else
                --pdp_idx;
        }
        else
            total_size_found += PML4_ENTRY_SIZE;
    }
    if (total_size_found < size)
        pml4_idx = 0;
    else
        --pml4_idx;
    
    *vaddr_out = VADDR_GET(pml4_idx, pdp_idx, pd_idx, pt_idx);
    return total_size_found;
}

uint64_t kernel_get_paddr(uint64_t vaddr)
{
    return pml4_get_paddr(kernel_pml4, vaddr);
}

uint64_t pml4_get_paddr(page_table_t pml4, uint64_t vaddr)
{
    page_table_t pt;
    uint64_t idx;
    page_table_entry_t entry;

    idx = VADDR_TO_PML4_IDX(vaddr);
    entry = pml4[idx];
    if (!entry.present)    
        return 0;
    
    idx = VADDR_TO_PDP_IDX(vaddr);
    pt = (page_table_t) kernel_map_temporary_page(get_pte_address(&entry), PAGE_ACCESS_RO, PL0);
    entry = pt[idx];
    kernel_unmap_temporary_page((uint64_t) pt);
    if (!entry.present)
        return 0;
    
    idx = VADDR_TO_PD_IDX(vaddr);
    pt = (page_table_t) kernel_map_temporary_page(get_pte_address(&entry), PAGE_ACCESS_RO, PL0);
    entry = pt[idx];
    kernel_unmap_temporary_page((uint64_t) pt);
    if (!entry.present)
        return 0;
    
    idx = VADDR_TO_PT_IDX(vaddr);
    pt = (page_table_t) kernel_map_temporary_page(get_pte_address(&entry), PAGE_ACCESS_RO, PL0);
    entry = pt[idx];
    kernel_unmap_temporary_page((uint64_t) pt);
    if (!entry.present)
        return 0;
    
    return (get_pte_address(&entry) + (vaddr & 0x0000000000000FFF));
}

static void delete_pd(page_table_t pd, uint64_t pd_paddr, uint64_t pdp_idx, uint64_t pml4_idx)
{
    uint64_t pd_idx, vaddr;
    page_table_entry_t entry;

    for 
    (
        pd_idx = 0, vaddr = VADDR_GET(pml4_idx, pdp_idx, 0 , 0);
        pd_idx < MAX_PAGE_TABLE_ENTRIES && vaddr < KERNEL_HEAP_START_ADDR;
        pd_idx++, vaddr = VADDR_GET(pml4_idx, pdp_idx, pd_idx, 0)
    )
    {
        entry = pd[pd_idx];
        if (entry.present)
            free_page(get_pte_address(&entry));
    }
    free_page(pd_paddr);
}

static void delete_pdp(page_table_t pdp, uint64_t pdp_paddr, uint64_t pml4_idx)
{
    uint64_t pdp_idx, pd_paddr, vaddr;
    page_table_entry_t entry;
    page_table_t pd;

    for 
    (
        pdp_idx = 0, vaddr = VADDR_GET(pml4_idx, 0, 0, 0);
        pdp_idx < MAX_PAGE_TABLE_ENTRIES && vaddr < KERNEL_HEAP_START_ADDR;
        pdp_idx++, vaddr = VADDR_GET(pml4_idx, pdp_idx, 0, 0)
    )
    {
        entry = pdp[pdp_idx];
        if (entry.present)
        {
            pd_paddr = get_pte_address(&entry);
            pd = (page_table_t) kernel_map_temporary_page(pd_paddr, PAGE_ACCESS_RO, PL0);
            delete_pd(pd, pd_paddr, pdp_idx, pml4_idx);
            kernel_unmap_temporary_page((uint64_t) pd);
        }
    }
    free_page(pdp_paddr);
}

uint64_t delete_pml4(page_table_t pml4, uint64_t pml4_paddr)
{
    uint64_t pml4_idx, pdp_paddr, vaddr;
    page_table_entry_t entry;
    page_table_t pdp;

    for 
    (
        pml4_idx = 0, vaddr = VADDR_GET(0, 0, 0, 0); 
        pml4_idx < MAX_PAGE_TABLE_ENTRIES && vaddr < KERNEL_HEAP_START_ADDR;
        pml4_idx++, vaddr = VADDR_GET(pml4_idx, 0, 0, 0)
    )
    {
        entry = pml4[pml4_idx];
        if (entry.present)
        {
            pdp_paddr = get_pte_address(&entry);
            pdp = (page_table_t) kernel_map_temporary_page(pdp_paddr, PAGE_ACCESS_RO, PL0);
            delete_pdp(pdp, pdp_paddr, pml4_idx);
            kernel_unmap_temporary_page((uint64_t) pdp);
        }
    }
    free_page(pml4_paddr);

    return vaddr;
}

static void merge_pt_with_pt(page_table_t src, page_table_t dest, uint64_t vaddr)
{
    uint64_t idx;
    page_table_entry_t src_entry, dest_entry;

    for (idx = VADDR_TO_PT_IDX(vaddr); idx < MAX_PAGE_TABLE_ENTRIES; idx++)
    {
        src_entry = src[idx];
        if (src_entry.present)
        {
            dest_entry = dest[idx];
            if (!dest_entry.present)
            {
                dest[idx] = src_entry;
                dest[idx].accessed = 0;
            }
        }
    }
}

static void merge_pd_with_pd(page_table_t src, page_table_t dest, uint64_t vaddr)
{
    uint64_t idx;
    page_table_entry_t src_entry, dest_entry;
    page_table_t src_pt, dest_pt;

    for (idx = VADDR_TO_PD_IDX(vaddr); idx < MAX_PAGE_TABLE_ENTRIES; idx++)
    {
        src_entry = src[idx];
        if (src_entry.present)
        {
            dest_entry = dest[idx];
            if (dest_entry.present)
            {
                src_pt = (page_table_t) kernel_map_temporary_page(get_pte_address(&src_entry), PAGE_ACCESS_RO, PL0);
                dest_pt = (page_table_t) kernel_map_temporary_page(get_pte_address(&dest_entry), PAGE_ACCESS_RW, PL0);
                merge_pt_with_pt(src_pt, dest_pt, vaddr);
                kernel_unmap_temporary_page((uint64_t) src_pt);
                kernel_unmap_temporary_page((uint64_t) dest_pt);
            }
            else
            {
                dest[idx] = src_entry;
                dest[idx].accessed = 0;
            }
            vaddr += PD_ENTRY_SIZE;
        }
    }
}

static void merge_pdp_with_pdp(page_table_t src, page_table_t dest, uint64_t vaddr)
{
    uint64_t idx;
    page_table_entry_t src_entry, dest_entry;
    page_table_t src_pd, dest_pd;

    for (idx = VADDR_TO_PDP_IDX(vaddr); idx < MAX_PAGE_TABLE_ENTRIES; idx++)
    {
        src_entry = src[idx];
        if (src_entry.present)
        {
            dest_entry = dest[idx];
            if (dest_entry.present)
            {
                src_pd = (page_table_t) kernel_map_temporary_page(get_pte_address(&src_entry), PAGE_ACCESS_RO, PL0);
                dest_pd = (page_table_t) kernel_map_temporary_page(get_pte_address(&dest_entry), PAGE_ACCESS_RW, PL0);
                merge_pd_with_pd(src_pd, dest_pd, vaddr);
                kernel_unmap_temporary_page((uint64_t) src_pd);
                kernel_unmap_temporary_page((uint64_t) dest_pd);
            }
            else
            {
                dest[idx] = src_entry;
                dest[idx].accessed = 0;
            }
            vaddr += PDP_ENTRY_SIZE;
        }
    }
}

static void merge_pml4_with_pml4(page_table_t src, page_table_t dest, uint64_t vaddr)
{
    uint64_t idx;
    page_table_entry_t src_entry, dest_entry;
    page_table_t src_pdp, dest_pdp;

    for (idx = VADDR_TO_PML4_IDX(vaddr); idx < MAX_PAGE_TABLE_ENTRIES; idx++)
    {
        src_entry = src[idx];
        if (src_entry.present)
        {
            dest_entry = dest[idx];
            if (dest_entry.present)
            {
                src_pdp = (page_table_t) kernel_map_temporary_page(get_pte_address(&src_entry), PAGE_ACCESS_RO, PL0);
                dest_pdp = (page_table_t) kernel_map_temporary_page(get_pte_address(&dest_entry), PAGE_ACCESS_RW, PL0);
                merge_pdp_with_pdp(src_pdp, dest_pdp, vaddr);
                kernel_unmap_temporary_page((uint64_t) src_pdp);
                kernel_unmap_temporary_page((uint64_t) dest_pdp);
            }
            else
            {
                dest[idx] = src_entry;
                dest[idx].accessed = 0;
            }
            vaddr += PML4_ENTRY_SIZE;
        }
    }
}

void kernel_inject_pml4(page_table_t pml4)
{
    merge_pml4_with_pml4(kernel_pml4, pml4, KERNEL_HEAP_START_ADDR);
}