#include <syscall.h>
#include "../kernel/ext/pci.h"

#define SPPFUNC __attribute__((section(".exposed"))) void

int main(int argc, char** argv)
{
    pci_export_list_t cool_export_list;
    int devices_found = syscall(SYS_pci_find, 0x1, 0x6, 0x1, &cool_export_list);
    while (1);
    return 0;
}