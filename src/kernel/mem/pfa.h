#ifndef __PFA_H__
#define __PFA_H__

#include <stdint.h>
#include "../utils/bitmap.h"

uint64_t pfa_request_page(void);
uint64_t pfa_request_pages(uint64_t num);
void pfa_lock_page(uint64_t page_addr);
void pfa_free_page(uint64_t page_addr);
void pfa_lock_pages(uint64_t page_addr, uint64_t num);
void pfa_free_pages(uint64_t page_addr, uint64_t num);
void pfa_restore(bitmap_t* current_bitmap);
void pfa_init(void);
bitmap_t* pfa_get_page_bitmap(void);
uint64_t pfa_get_free_memory(void);
uint64_t pfa_get_used_memory(void);

#endif