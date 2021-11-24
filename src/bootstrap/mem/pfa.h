#ifndef __PFA_H__
#define __PFA_H__

#include <stdint.h>

uint64_t request_page(void);
uint64_t request_pages(uint64_t num);
void lock_page(uint64_t page_addr);
void free_page(uint64_t page_addr);
void lock_pages(uint64_t page_addr, uint64_t num);
void free_pages(uint64_t page_addr, uint64_t num);
void init_pfa(void);

#endif