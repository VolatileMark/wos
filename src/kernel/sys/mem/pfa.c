#include "pfa.h"
#include "mmap.h"
#include "paging.h"
#include <math.h>
#include <mem.h>

#define PFA_GET_PAGE_IDX(addr) (addr / SIZE_4KB)

static bitmap_t page_bitmap;
static uint64_t last_page_index;
static uint64_t free_memory, used_memory;

uint64_t pfa_request_page(void)
{
    uint64_t address;
    for (; last_page_index < page_bitmap.size * 8; last_page_index++)
    {
        if (!bitmap_get(&page_bitmap, last_page_index))
        {
            address = (last_page_index * SIZE_4KB);
            pfa_lock_page(address);
            return address;
        }
    }
    return 0;
}

uint64_t pfa_request_pages(uint64_t num)
{
    uint64_t found;
    uint64_t address;
    
    for (found = 0, address = 0; last_page_index < page_bitmap.size * 8 && found < num; last_page_index++)
    {
        if (!bitmap_get(&page_bitmap, last_page_index))
        {
            ++found;
            if (address == 0)
                address = (last_page_index * SIZE_4KB);
        }
        else
        {
            found = 0;
            address = 0;
        }
    }

    if (found == num)
        pfa_lock_pages(address, num);
    
    return address;
}

void pfa_lock_page(uint64_t page_addr)
{
    uint64_t index;
    index = PFA_GET_PAGE_IDX(page_addr);
    if (!bitmap_get(&page_bitmap, index))
    {
        bitmap_set(&page_bitmap, PFA_GET_PAGE_IDX(page_addr), 1);
        used_memory += SIZE_4KB;
        free_memory -= SIZE_4KB;
    }
}

void pfa_free_page(uint64_t page_addr)
{
    uint64_t index;
    index = PFA_GET_PAGE_IDX(page_addr);
    if (bitmap_get(&page_bitmap, index))
    {
        bitmap_set(&page_bitmap, index, 0);
        free_memory += SIZE_4KB;
        used_memory -= SIZE_4KB;
    }
    if (last_page_index > index)
        last_page_index = index;
}

void pfa_lock_pages(uint64_t page_addr, uint64_t num)
{
    for (; num > 0; num--, page_addr += SIZE_4KB)
        pfa_lock_page(page_addr);
}

void pfa_free_pages(uint64_t page_addr, uint64_t num)
{
    for (; num > 0; num--, page_addr += SIZE_4KB)
        pfa_free_page(page_addr);
}

void pfa_init(void)
{
    uint64_t total_memory, bitmap_size, bitmap_paddr, bitmap_vaddr;

    total_memory = mmap_get_total_memory_of_type(MULTIBOOT_MEMORY_AVAILABLE);
    free_memory = total_memory - used_memory;

    /* Initialize new bitmap */
    bitmap_size = ceil(((double) total_memory) / SIZE_4KB / 8.0);
    bitmap_paddr = pfa_request_pages(ceil((double) bitmap_size / SIZE_4KB));
    kernel_get_next_vaddr(bitmap_size, &bitmap_vaddr);
    paging_map_memory(bitmap_paddr, bitmap_vaddr, bitmap_size, PAGE_ACCESS_RW, PL0);
    memset((void*) bitmap_vaddr, 0, bitmap_size);

    /* Copy the old bitmap over */
    pfa_free_pages(alignd((uint64_t) page_bitmap.buffer, SIZE_4KB), ceil((double) page_bitmap.size / SIZE_4KB));
    memcpy((void*) bitmap_vaddr, page_bitmap.buffer, page_bitmap.size);

    /* Set new bitmap */
    page_bitmap.buffer = (uint8_t*) bitmap_vaddr;
    page_bitmap.size = bitmap_size;
}

void pfa_restore(bitmap_t* current_bitmap)
{
    uint64_t i;

    last_page_index = 0;
    page_bitmap.buffer = current_bitmap->buffer;
    page_bitmap.size = current_bitmap->size;

    used_memory = 0;
    for (i = 0; i < page_bitmap.size * 8; i++)
    {
        if (bitmap_get(&page_bitmap, i))
            used_memory += SIZE_4KB;
    }
}

bitmap_t* pfa_get_page_bitmap(void)
{
    return &page_bitmap;
}

uint64_t pfa_get_free_memory(void)
{
    return free_memory;
}

uint64_t pfa_get_used_memory(void)
{
    return used_memory;
}