#include "paging.h"
#include "pfa.h"
#include "../../kernel.h"
#include <math.h>
#include <mem.h>

#define KERNEL_PML4_VADDR VADDR_GET_TEMPORARY(0)

static page_table_t kernel_pml4;
static page_table_t kernel_tmp_pt;
static uint64_t kernel_tmp_index;
static uint64_t kernel_pml4_paddr;

uint64_t paging_get_kernel_pml4_paddr(void)
{
    return kernel_pml4_paddr;
}

page_table_t paging_get_kernel_pml4(void)
{
    return kernel_pml4;
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
    if (kernel_tmp_index >= PT_MAX_ENTRIES)
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

static void pte_create(page_table_t table, uint64_t index, uint64_t paddr, page_access_type_t access, privilege_level_t privilege_level)
{
    page_table_entry_t* entry;
    entry = &table[index];
    PTE_CLEAR(entry);
    pte_set_address(entry, paddr);
    entry->present = (((uint64_t) access) & 0b0100) >> 2;
    entry->allow_writes = (((uint64_t) access) & 0b0001);
    entry->allow_user_access = (privilege_level == PL3);
    /* entry->no_execute = (((uint64_t) access) & 0b0010) >> 1 */;
}

uint64_t kernel_map_temporary_page(uint64_t paddr, page_access_type_t access, privilege_level_t privilege_level)
{
    uint64_t index;
    index = paging_get_next_tmp_index();
    pte_create(kernel_tmp_pt, index, paddr, access, privilege_level);
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
    pte_invalidate(vaddr);
}

void paging_init(void)
{
    kernel_pml4 = (page_table_t) KERNEL_PML4_VADDR;
    kernel_tmp_pt = (page_table_t) VADDR_GET_TEMPORARY(1);
    kernel_tmp_index = 2;
    kernel_pml4_paddr = kernel_get_paddr(KERNEL_PML4_VADDR);
}

uint64_t kernel_map_memory(uint64_t paddr, uint64_t vaddr, uint64_t size, page_access_type_t access, privilege_level_t privilege_level)
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
    uint64_t pt_idx;
    uint64_t unmapped_size;

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
    uint64_t pd_idx;
    page_table_t pt;
    page_table_entry_t entry;
    uint64_t i;
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
        pt_vaddr = kernel_map_temporary_page(pt_paddr, PAGE_ACCESS_RW, PL0);

        pt = (page_table_t) pt_vaddr;
        unmapped_size = pt_unmap_memory(pt, vaddr, size - total_unmapped_size);

        for (i = 0; i < PT_MAX_ENTRIES && !pt[i].present; i++);

        kernel_unmap_temporary_page(pt_vaddr);

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
    uint64_t pdp_idx;
    page_table_t pd;
    page_table_entry_t entry;
    uint64_t i;
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
        pd_vaddr = kernel_map_temporary_page(pd_paddr, PAGE_ACCESS_RW, PL0);

        pd = (page_table_t) pd_vaddr;
        unmapped_size = pd_unmap_memory(pd, vaddr, size - total_unmapped_size);

        for (i = 0; i < PT_MAX_ENTRIES && !pd[i].present; i++);

        kernel_unmap_temporary_page(pd_vaddr);

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
    uint64_t pml4_idx;
    page_table_t pdp;
    page_table_entry_t entry;
    uint64_t i;
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
        pdp_vaddr = kernel_map_temporary_page(pdp_paddr, PAGE_ACCESS_RW, PL0);

        pdp = (page_table_t) pdp_vaddr;
        unmapped_size = pdp_unmap_memory(pdp, vaddr, size - total_unmapped_size);

        for (i = 0; i < PT_MAX_ENTRIES && !pdp[i].present; i++);

        kernel_unmap_temporary_page(pdp_vaddr);

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
        if (pt[pt_idx].present)
            return 0;
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
    uint64_t pd_idx;
    page_table_t pt;
    page_table_entry_t entry;
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
            if (pt_paddr == 0)
                return 0;
            pt_vaddr = kernel_map_temporary_page(pt_paddr, PAGE_ACCESS_RW, privilege_level);
            memset((void*) pt_vaddr, 0, SIZE_4KB);
            pte_create(pd, pd_idx, pt_paddr, PAGE_ACCESS_RW, privilege_level);
        }
        else
        {
            pt_paddr = pte_get_address(&entry);
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
    page_access_type_t access,
    privilege_level_t privilege_level
)
{
    uint64_t pdp_idx;
    page_table_t pd;
    page_table_entry_t entry;
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
            if (pd_paddr == 0)
                return 0;
            pd_vaddr = kernel_map_temporary_page(pd_paddr, PAGE_ACCESS_RW, privilege_level);
            memset((void*) pd_vaddr, 0, SIZE_4KB);
            pte_create(pdp, pdp_idx, pd_paddr, PAGE_ACCESS_RW, privilege_level);
        }
        else
        {
            pd_paddr = pte_get_address(&entry);
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
    page_access_type_t access, 
    privilege_level_t privilege_level
)
{
    uint64_t pml4_idx;
    page_table_t pdp;
    page_table_entry_t entry;
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
            if (pdp_paddr == 0)
                return 0;
            pdp_vaddr = kernel_map_temporary_page(pdp_paddr, PAGE_ACCESS_RW, privilege_level);
            memset((void*) pdp_vaddr, 0, SIZE_4KB);
            pte_create(pml4, pml4_idx, pdp_paddr, PAGE_ACCESS_RW, privilege_level);
        }
        else
        {
            pdp_paddr = pte_get_address(&entry);
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
    return pml4_get_next_vaddr(kernel_pml4, (uint64_t) &_end_addr, size, vaddr_out);
}

uint64_t pml4_get_next_vaddr(page_table_t pml4, uint64_t vaddr_start, uint64_t size, uint64_t* vaddr_out)
{
    uint64_t pml4_idx, pdp_idx, pd_idx, pt_idx;
    uint64_t total_size_found;
    page_table_t pdp, pd, pt;
    page_table_entry_t entry;

    size = alignu(size, SIZE_4KB);
    vaddr_start = alignd(vaddr_start, PT_ENTRY_SIZE);
    total_size_found = 0;
    pml4_idx = VADDR_TO_PML4_IDX(vaddr_start);
    pdp_idx = VADDR_TO_PDP_IDX(vaddr_start);
    pd_idx = VADDR_TO_PD_IDX(vaddr_start);
    pt_idx = VADDR_TO_PT_IDX(vaddr_start);

    for
    (
        ;
        total_size_found < size && pml4_idx < PT_MAX_ENTRIES;
        pml4_idx++
    )
    {
        entry = pml4[pml4_idx];
        if (entry.present)
        {
            pdp = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RO, PL0);
            for
            (
                ;
                total_size_found < size && pdp_idx < PT_MAX_ENTRIES;
                pdp_idx++
            )
            {
                entry = pdp[pdp_idx];
                if (entry.present)
                {
                    pd = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RO, PL0);
                    for
                    (
                        ;
                        total_size_found < size && pd_idx < PT_MAX_ENTRIES;
                        pd_idx++
                    )
                    {
                        entry = pd[pd_idx];
                        if (entry.present)
                        {
                            pt = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RO, PL0);
                            for
                            (
                                ;
                                total_size_found < size && pt_idx < PT_MAX_ENTRIES;
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
    page_table_t pdp, pd, pt;
    uint64_t pml4_idx, pdp_idx, pd_idx, pt_idx;
    page_table_entry_t entry;

    pml4_idx = VADDR_TO_PML4_IDX(vaddr);
    entry = pml4[pml4_idx];
    if (!entry.present)    
        return 0;
    
    pdp_idx = VADDR_TO_PDP_IDX(vaddr);
    pdp = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RO, PL0);
    entry = pdp[pdp_idx];
    kernel_unmap_temporary_page((uint64_t) pdp);
    if (!entry.present)
        return 0;
    
    pd_idx = VADDR_TO_PD_IDX(vaddr);
    pd = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RO, PL0);
    entry = pd[pd_idx];
    kernel_unmap_temporary_page((uint64_t) pd);
    if (!entry.present)
        return 0;
    
    pt_idx = VADDR_TO_PT_IDX(vaddr);
    pt = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RO, PL0);
    entry = pt[pt_idx];
    kernel_unmap_temporary_page((uint64_t) pt);
    if (!entry.present)
        return 0;
    
    return (pte_get_address(&entry) + GET_ADDR_OFFSET(vaddr));
}

static void delete_pd(page_table_t pd, uint64_t pd_paddr, uint64_t pdp_idx, uint64_t pml4_idx)
{
    uint64_t pd_idx, vaddr;
    page_table_entry_t entry;

    for 
    (
        pd_idx = 0, vaddr = VADDR_GET(pml4_idx, pdp_idx, 0 , 0);
        pd_idx < PT_MAX_ENTRIES && vaddr < KERNEL_HEAP_START_ADDR;
        pd_idx++, vaddr = VADDR_GET(pml4_idx, pdp_idx, pd_idx, 0)
    )
    {
        entry = pd[pd_idx];
        if (entry.present)
            pfa_free_page(pte_get_address(&entry));
    }
    pfa_free_page(pd_paddr);
}

static void delete_pdp(page_table_t pdp, uint64_t pdp_paddr, uint64_t pml4_idx)
{
    uint64_t pdp_idx, pd_paddr, vaddr;
    page_table_entry_t entry;
    page_table_t pd;

    for 
    (
        pdp_idx = 0, vaddr = VADDR_GET(pml4_idx, 0, 0, 0);
        pdp_idx < PT_MAX_ENTRIES && vaddr < KERNEL_HEAP_START_ADDR;
        pdp_idx++, vaddr = VADDR_GET(pml4_idx, pdp_idx, 0, 0)
    )
    {
        entry = pdp[pdp_idx];
        if (entry.present)
        {
            pd_paddr = pte_get_address(&entry);
            pd = (page_table_t) kernel_map_temporary_page(pd_paddr, PAGE_ACCESS_RO, PL0);
            delete_pd(pd, pd_paddr, pdp_idx, pml4_idx);
            kernel_unmap_temporary_page((uint64_t) pd);
        }
    }
    pfa_free_page(pdp_paddr);
}

uint64_t pml4_delete(page_table_t pml4, uint64_t pml4_paddr)
{
    uint64_t pml4_idx, pdp_paddr, vaddr;
    page_table_entry_t entry;
    page_table_t pdp;

    for 
    (
        pml4_idx = 0, vaddr = VADDR_GET(0, 0, 0, 0); 
        pml4_idx < PT_MAX_ENTRIES && vaddr < KERNEL_HEAP_START_ADDR;
        pml4_idx++, vaddr = VADDR_GET(pml4_idx, 0, 0, 0)
    )
    {
        entry = pml4[pml4_idx];
        if (entry.present)
        {
            pdp_paddr = pte_get_address(&entry);
            pdp = (page_table_t) kernel_map_temporary_page(pdp_paddr, PAGE_ACCESS_RO, PL0);
            delete_pdp(pdp, pdp_paddr, pml4_idx);
            kernel_unmap_temporary_page((uint64_t) pdp);
        }
    }
    pfa_free_page(pml4_paddr);

    return vaddr;
}

static void merge_pt_with_pt(page_table_t src, page_table_t dest, uint64_t vaddr)
{
    uint64_t idx;
    page_table_entry_t src_entry, dest_entry;

    for (idx = VADDR_TO_PT_IDX(vaddr); idx < PT_MAX_ENTRIES; idx++)
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

    for (idx = VADDR_TO_PD_IDX(vaddr); idx < PT_MAX_ENTRIES; idx++)
    {
        src_entry = src[idx];
        if (src_entry.present)
        {
            dest_entry = dest[idx];
            if (dest_entry.present)
            {
                src_pt = (page_table_t) kernel_map_temporary_page(pte_get_address(&src_entry), PAGE_ACCESS_RO, PL0);
                dest_pt = (page_table_t) kernel_map_temporary_page(pte_get_address(&dest_entry), PAGE_ACCESS_RW, PL0);
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

    for (idx = VADDR_TO_PDP_IDX(vaddr); idx < PT_MAX_ENTRIES; idx++)
    {
        src_entry = src[idx];
        if (src_entry.present)
        {
            dest_entry = dest[idx];
            if (dest_entry.present)
            {
                src_pd = (page_table_t) kernel_map_temporary_page(pte_get_address(&src_entry), PAGE_ACCESS_RO, PL0);
                dest_pd = (page_table_t) kernel_map_temporary_page(pte_get_address(&dest_entry), PAGE_ACCESS_RW, PL0);
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

    for (idx = VADDR_TO_PML4_IDX(vaddr); idx < PT_MAX_ENTRIES; idx++)
    {
        src_entry = src[idx];
        if (src_entry.present)
        {
            dest_entry = dest[idx];
            if (dest_entry.present)
            {
                src_pdp = (page_table_t) kernel_map_temporary_page(pte_get_address(&src_entry), PAGE_ACCESS_RO, PL0);
                dest_pdp = (page_table_t) kernel_map_temporary_page(pte_get_address(&dest_entry), PAGE_ACCESS_RW, PL0);
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

int kernel_set_pte_flag(uint64_t vaddr, page_flag_t flag)
{
    return pml4_set_pte_flag(kernel_pml4, vaddr, flag);
}

int pml4_set_pte_flag(page_table_t pml4, uint64_t vaddr, page_flag_t flag)
{
    page_table_t pt;
    page_table_entry_t entry;
    uint64_t pt_idx;

    pt_idx = VADDR_TO_PML4_IDX(vaddr);
    entry = pml4[pt_idx];
    if (!entry.present)
        return -1;
    pt = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RW, PL0);

    pt_idx = VADDR_TO_PDP_IDX(vaddr);
    entry = pt[pt_idx];
    kernel_unmap_temporary_page((uint64_t) pt);
    if (!entry.present)
        return -1;
    pt = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RW, PL0);

    pt_idx = VADDR_TO_PD_IDX(vaddr);
    entry = pt[pt_idx];
    kernel_unmap_temporary_page((uint64_t) pt);
    if (!entry.present)
        return -1;
    pt = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RW, PL0);

    pt_idx = VADDR_TO_PT_IDX(vaddr);
    entry = pt[pt_idx];
    if (!entry.present)
    {
        kernel_unmap_temporary_page((uint64_t) pt);
        return -1;
    }
    *((uint64_t*) &entry) |= ((uint64_t) 1 << flag);
    kernel_unmap_temporary_page((uint64_t) pt);
    
    return 0;
}

int kernel_reset_pte_flag(uint64_t vaddr, page_flag_t flag)
{
    return pml4_set_pte_flag(kernel_pml4, vaddr, flag);
}

int pml4_reset_pte_flag(page_table_t pml4, uint64_t vaddr, page_flag_t flag)
{
    page_table_t pt;
    page_table_entry_t entry;
    uint64_t pt_idx;

    pt_idx = VADDR_TO_PML4_IDX(vaddr);
    entry = pml4[pt_idx];
    if (!entry.present)
        return -1;
    pt = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RW, PL0);

    pt_idx = VADDR_TO_PDP_IDX(vaddr);
    entry = pt[pt_idx];
    kernel_unmap_temporary_page((uint64_t) pt);
    if (!entry.present)
        return -1;
    pt = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RW, PL0);

    pt_idx = VADDR_TO_PD_IDX(vaddr);
    entry = pt[pt_idx];
    kernel_unmap_temporary_page((uint64_t) pt);
    if (!entry.present)
        return -1;
    pt = (page_table_t) kernel_map_temporary_page(pte_get_address(&entry), PAGE_ACCESS_RW, PL0);

    pt_idx = VADDR_TO_PT_IDX(vaddr);
    entry = pt[pt_idx];
    if (!entry.present)
    {
        kernel_unmap_temporary_page((uint64_t) pt);
        return -1;
    }
    *((uint64_t*) &entry) &= ~((uint64_t) 1 << flag);
    kernel_unmap_temporary_page((uint64_t) pt);

    return 0;
}

int kernel_flag_memory_area(uint64_t vaddr, uint64_t size, page_flag_t flag)
{
    return pml4_flag_memory_area(kernel_pml4, vaddr, size, flag);
}

int pml4_flag_memory_area(page_table_t pml4, uint64_t vaddr, uint64_t size, page_flag_t flag)
{
    vaddr = alignd(vaddr, SIZE_4KB);
    size = alignu(size, SIZE_4KB);

    while (size > 0)
    {
        if (pml4_set_pte_flag(pml4, vaddr, flag))
            return -1;
        size -= SIZE_4KB;
        vaddr += SIZE_4KB;
    }

    return 0;
}

int kernel_unflag_memory_area(uint64_t vaddr, uint64_t size, page_flag_t flag)
{
    return pml4_unflag_memory_area(kernel_pml4, vaddr, size, flag);
}

int pml4_unflag_memory_area(page_table_t pml4, uint64_t vaddr, uint64_t size, page_flag_t flag)
{
    vaddr = alignd(vaddr, SIZE_4KB);
    size = alignu(size, SIZE_4KB);

    while (size > 0)
    {
        if (pml4_reset_pte_flag(pml4, vaddr, flag))
            return -1;
        size -= SIZE_4KB;
        vaddr += SIZE_4KB;
    }

    return 0;
}