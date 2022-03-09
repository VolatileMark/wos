#include "pci.h"
#include "../power/acpi.h"
#include "../../../mem/paging.h"
#include "../../../utils/log.h"
#include "../../../utils/alloc.h"
#include <stddef.h>
#include <mem.h>

#define trace_pci(msg, ...) trace("PCIB", msg, ##__VA_ARGS__)

static pci_devices_list_t devices_list;

static void pci_enumerate_function(uint64_t address, uint64_t num)
{
    pci_header_common_t* header;
    pci_devices_list_entry_t* new_entry;
    uint64_t function_address;
    
    function_address = address + (num << 12);
    header = (pci_header_common_t*) kernel_map_temporary_page(function_address, PAGE_ACCESS_RX, PL0);
    if (header->device_id == 0 || header->device_id == 0xFFFF)
        return;
    
    new_entry = malloc(sizeof(pci_devices_list_entry_t));
    new_entry->next = NULL;
    new_entry->header_paddr = function_address;
    if (devices_list.tail == NULL)
        devices_list.head = new_entry;
    else
        devices_list.tail->next = new_entry;
    devices_list.tail = new_entry;

    info("PCI device %x:%x found of type %x:%x:%x", header->vendor_id, header->device_id, header->class_code, header->subclass, header->program_interface);

    kernel_unmap_temporary_page((uint64_t) header);
}

static void pci_enumerate_device(uint64_t address, uint64_t num)
{
    pci_header_common_t* header;
    uint64_t function;
    uint64_t device_address;
    
    device_address = address + (num << 15);
    header = (pci_header_common_t*) kernel_map_temporary_page(device_address, PAGE_ACCESS_RX, PL0);
    if (header->device_id == 0 || header->device_id == 0xFFFF)
        return;
    for (function = 0; function < 8; function++)
        pci_enumerate_function(device_address, function);
    kernel_unmap_temporary_page((uint64_t) header);
}

static void pci_enumerate_bus(uint64_t address, uint64_t num)
{
    pci_header_common_t* header;
    uint64_t device;
    uint64_t bus_address;
    
    bus_address = address + (num << 20);
    header = (pci_header_common_t*) kernel_map_temporary_page(bus_address, PAGE_ACCESS_RX, PL0);
    if (header->device_id == 0 || header->device_id == 0xFFFF)
        return;
    for (device = 0; device < 32; device++)
        pci_enumerate_device(bus_address, device);
    kernel_unmap_temporary_page((uint64_t) header);
}

static void pci_enumerate(mcfg_t* mcfg)
{
    mcfg_entry_t* entry;
    uint64_t entries, entry_num, bus;
    
    entries = (mcfg->sdt_header.length - sizeof(mcfg_t)) / sizeof(mcfg_entry_t);
    for (entry_num = 0; entry_num < entries;entry_num++)
    {
        entry = &mcfg->entries[entry_num];
        for (bus = entry->pci_start_bus_num; bus < entry->pci_end_bus_num; bus++)
            pci_enumerate_bus(entry->base_address, bus);
    }
}

int pci_init(void)
{
    mcfg_t* mcfg;
    
    mcfg = (mcfg_t*) acpi_find_table(MCFG_SIG);
    if (mcfg == NULL)
    {
        trace_pci("MCFG table not found");
        return -1;
    }
    memset(&devices_list, 0, sizeof(pci_devices_list_t));
    pci_enumerate(mcfg);
    return 0;
}

pci_devices_list_t* pci_find_devices(int class, int subclass, int program_interface)
{
    pci_devices_list_entry_t* entry = devices_list.head;
    pci_devices_list_entry_t* new;
    pci_header_common_t* header;
    pci_devices_list_t* devices_found;
    
    devices_found = calloc(1, sizeof(pci_devices_list_t));
    if (devices_found == NULL)
        return NULL;

    while (entry != NULL)
    {
        if (entry->header_paddr != 0)
        {
            header = (pci_header_common_t*) (kernel_map_temporary_page(entry->header_paddr, PAGE_ACCESS_RX, PL0) + GET_ADDR_OFFSET(entry->header_paddr));
            if 
            (
                (class == -1 || header->class_code == (uint8_t) class) &&
                (subclass == -1 || header->subclass == (uint8_t) subclass) &&
                (program_interface == -1 || header->program_interface == (uint8_t) program_interface)
            )
            {
                new = malloc(sizeof(pci_devices_list_entry_t));
                if (new == NULL)
                {
                    kernel_unmap_temporary_page((uint64_t) header);
                    pci_delete_devices_list(devices_found);
                    free(devices_found);
                    return NULL;
                }
                new->next = NULL;
                new->header_paddr = entry->header_paddr;
                if (devices_found->tail == NULL)
                    devices_found->head = new;
                else
                    devices_found->tail->next = new;
                devices_found->tail = new;
            }
            kernel_unmap_temporary_page((uint64_t) header);
        }
        entry = entry->next;
    }

    return devices_found;
}

void pci_delete_devices_list(pci_devices_list_t* list)
{
    pci_devices_list_entry_t* tmp;
    pci_devices_list_entry_t* entry;
    
    entry = list->head;
    while (entry != NULL)
    {
        tmp = entry->next;
        free(entry);
        entry = tmp;
    }
    
    memset(list, 0, sizeof(pci_devices_list_t));    
}