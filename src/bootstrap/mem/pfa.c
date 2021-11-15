#include "pfa.h"
#include "mmap.h"
#include "../utils/bitmap.h"
#include "../utils/constants.h"
#include "../external/multiboot2.h"
#include <math.h>
#include <stdint.h>
#include <stddef.h>

#define PFA_BITMAP_BUFFER_ADDR 0x00200000
#define PFA_MAX_MEM_SIZE 0x40000000 /* 1GB */
#define PFA_GET_PAGE_IDX(addr) (addr / SIZE_4KB)

static bitmap_t page_bitmap;
static uint64_t last_page_index;

void* request_page(void)
{
    uint64_t i;
    for (i = ceil((double)((uint64_t) &_start_addr) / SIZE_4KB); i < page_bitmap.size * 8; i++)
    {
        if (bitmap_get(&page_bitmap, i) == 0)
        {
            void* address = (void*)(i * SIZE_4KB);
            lock_page((uint64_t) address);
            return address;
        }
    }
    return NULL;
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

void pfa_init(void)
{
    uint64_t total_memory = get_total_memory_of_type(MULTIBOOT_MEMORY_AVAILABLE);
    if (total_memory > PFA_MAX_MEM_SIZE)
        total_memory = PFA_MAX_MEM_SIZE;
    page_bitmap.size = ceil(((double) total_memory) / SIZE_4KB / 8.0);
    page_bitmap.buffer = (uint8_t*) PFA_BITMAP_BUFFER_ADDR;
    memset(page_bitmap.buffer, 0, page_bitmap.size);
}