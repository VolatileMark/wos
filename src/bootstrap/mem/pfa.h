#ifndef __PFA_H__
#define __PFA_H__

void* request_page(void);
void lock_page(uint64_t page_addr);
void free_page(uint64_t page_addr);
void lock_pages(uint64_t page_addr, uint64_t num);
void free_pages(uint64_t page_addr, uint64_t num);
void pfa_init(void);

#endif