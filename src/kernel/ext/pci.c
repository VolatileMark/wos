#include "pci.h"
#include "acpi.h"

void scan_pci(void)
{
    sdt_header_t* mcfg = find_acpi_table("MCFG");
    (void)(mcfg);
}