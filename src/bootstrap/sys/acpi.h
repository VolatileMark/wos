#ifndef __ACPI_H__
#define __ACPI_H__

#include <stdint.h>

int acpi_lock_sdt_pages(uint64_t rsdp_start_paddr, uint64_t* rsdp_size);

#endif
