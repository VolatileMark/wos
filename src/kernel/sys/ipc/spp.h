/* Cross Process Call */
#ifndef __SPP_H__
#define __SPP_H__

#include <stdint.h>
#include "../process.h"

void init_spp(void);
int expose_function(process_t* ps, uint64_t func_addr, const char* func_name);
void delete_exposed_functions(process_t* ps);
uint64_t request_func(process_t* ps, const char* name);

#endif