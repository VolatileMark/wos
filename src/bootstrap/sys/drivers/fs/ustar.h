#ifndef __USTAR_H__
#define __USTAR_H__

#include <stdint.h>

void ustar_set_address(uint64_t addr);
uint64_t ustar_lookup(char* name, uint64_t* out);

#endif
