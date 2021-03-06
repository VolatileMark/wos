#include "pfa.h"
#include "mmap.h"
#include "../../utils/constants.h"
#include <math.h>
#include <mem.h>

#define PFA_MAX_MEM_SIZE 0x08000000 /* 128MB */
#define PFA_GET_PAGE_IDX(addr) (addr / SIZE_4KB)

static bitmap_t page_bitmap;
static uint64_t last_page_index;
static __attribute__((aligned(SIZE_4KB))) uint8_t page_bitmap_buffer[SIZE_4KB];

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
    for 
    (
        found = 0, address = 0; 
        last_page_index < page_bitmap.size * 8 && found < num; 
        last_page_index++
    )
    {
        if (!bitmap_get(&page_bitmap, last_page_index))
        {
            ++found;
            if (address == 0) { address = (last_page_index * SIZE_4KB); }
        }
        else
        {
            found = 0;
            address = 0;
        }
    }

    if (found == num) { pfa_lock_pages(address, num); }
    
    return address;
}

void pfa_lock_page(uint64_t page_addr)
{
    bitmap_set(&page_bitmap, PFA_GET_PAGE_IDX(page_addr), 1);
}

void pfa_free_page(uint64_t page_addr)
{
    uint64_t index;
    index = PFA_GET_PAGE_IDX(page_addr);
    bitmap_set(&page_bitmap, index, 0);
    if (last_page_index > index) { last_page_index = index; }
}

void pfa_lock_pages(uint64_t page_addr, uint64_t num)
{
    for (; num > 0; num--, page_addr += SIZE_4KB) { pfa_lock_page(page_addr); }
}

void pfa_free_pages(uint64_t page_addr, uint64_t num)
{
    for (; num > 0; num--, page_addr += SIZE_4KB) { pfa_free_page(page_addr); }
}

void pfa_init(void)
{
    uint64_t total_memory;
    total_memory = mmap_get_total_memory_of_type(MULTIBOOT_MEMORY_AVAILABLE);
    if (total_memory > PFA_MAX_MEM_SIZE) { total_memory = PFA_MAX_MEM_SIZE; }
    page_bitmap.size = ceil(((double) total_memory) / SIZE_4KB / 8.0);
    page_bitmap.buffer = page_bitmap_buffer;
    memset(page_bitmap.buffer, 0, page_bitmap.size);
    pfa_lock_pages((uint64_t) page_bitmap.buffer, ceil((double) page_bitmap.size / SIZE_4KB));
    last_page_index = ceil((double)((uint64_t) &_end_addr) / SIZE_4KB);
}

bitmap_t* pfa_get_page_bitmap(void)
{
    return &page_bitmap;
}
