#include "pfa.h"
#include "mmap.h"
#include "../utils/constants.h"
#include "../utils/multiboot2.h"
#include <math.h>
#include <stddef.h>
#include <mem.h>

#define PFA_MAX_MEM_SIZE 0x08000000 /* 128MB */
#define PFA_GET_PAGE_IDX(addr) (addr / SIZE_4KB)

static bitmap_t page_bitmap;
static uint64_t last_page_index;
static __attribute__((aligned(SIZE_4KB))) uint8_t page_bitmap_buffer[SIZE_4KB];

uint64_t request_page(void)
{
    uint64_t address;
    for (; last_page_index < page_bitmap.size * 8; last_page_index++)
    {
        if (!bitmap_get(&page_bitmap, last_page_index))
        {
            address = (last_page_index * SIZE_4KB);
            lock_page(address);
            return address;
        }
    }
    return 0;
}

uint64_t request_pages(uint64_t num)
{
    uint64_t found;
    uint64_t address = 0;
    for (found = 0; last_page_index < page_bitmap.size * 8 && found < num; last_page_index++)
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

    if (address == 0)
        lock_pages(address, num);
    
    return address;
}

void lock_page(uint64_t page_addr)
{
    bitmap_set(&page_bitmap, PFA_GET_PAGE_IDX(page_addr), 1);
}

void free_page(uint64_t page_addr)
{
    uint64_t index = PFA_GET_PAGE_IDX(page_addr);
    bitmap_set(&page_bitmap, index, 0);
    if (last_page_index > index)
        last_page_index = index;
}

void lock_pages(uint64_t page_addr, uint64_t num)
{
    for (; num > 0; num--, page_addr += SIZE_4KB)
        lock_page(page_addr);
}

void free_pages(uint64_t page_addr, uint64_t num)
{
    for (; num > 0; num--, page_addr += SIZE_4KB)
        free_page(page_addr);
}

void init_pfa(void)
{
    uint64_t total_memory = get_total_memory_of_type(MULTIBOOT_MEMORY_AVAILABLE);
    if (total_memory > PFA_MAX_MEM_SIZE)
        total_memory = PFA_MAX_MEM_SIZE;
    page_bitmap.size = ceil(((double) total_memory) / SIZE_4KB / 8.0);
    page_bitmap.buffer = page_bitmap_buffer;
    memset(page_bitmap.buffer, 0, page_bitmap.size);
    lock_pages((uint64_t) page_bitmap.buffer, ceil((double) page_bitmap.size / SIZE_4KB));
    last_page_index = ceil((double)((uint64_t) &_end_addr) / SIZE_4KB);
}

bitmap_t* get_page_bitmap(void)
{
    return &page_bitmap;
}