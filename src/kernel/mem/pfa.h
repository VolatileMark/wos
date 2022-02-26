#ifndef __PFA_H__
#define __PFA_H__

#include <stdint.h>
#include "../utils/helpers/bitmap.h"

uint64_t request_page(void);
uint64_t request_pages(uint64_t num);
void lock_page(uint64_t page_addr);
void free_page(uint64_t page_addr);
void lock_pages(uint64_t page_addr, uint64_t num);
void free_pages(uint64_t page_addr, uint64_t num);
void restore_pfa(bitmap_t* current_bitmap);
void init_pfa(void);
bitmap_t* get_page_bitmap(void);
uint64_t get_free_memory(void);
uint64_t get_used_memory(void);

#endif