#ifndef __PCI_H__
#define __PCI_H__

#include <stdint.h>

struct pci_header_common
{
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t program_interface;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type : 7;
    uint8_t multiple_functions : 1;
    uint8_t bist;
} __attribute__((packed));
typedef struct pci_header_common pci_header_common_t;

struct pci_header_0x0
{
    pci_header_common_t common;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
    uint32_t cardbus_cis_pointer;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_address;
    uint8_t capabilities_pointer;
    uint8_t reserved0;
    uint16_t reserved1;
    uint32_t reserved2;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
} __attribute__((packed));
typedef struct pci_header_0x0 pci_header_0x0_t;

struct pci_header_0x1
{
    pci_header_common_t common;
    uint32_t bar0;
    uint32_t bar1;
    uint8_t primary_bus_number;
    uint8_t secondary_bus_number;
    uint8_t subordinate_bus_number;
    uint8_t secondary_latency_timer;
    uint8_t io_base_lo;
    uint8_t io_limit_lo;
    uint16_t secondary_status;
    uint16_t memory_base;
    uint16_t memory_limit;
    uint16_t prefetchable_memory_base_lo;
    uint16_t prefetchable_memory_limit_lo;
    uint32_t prefetchable_memory_base_hi;
    uint32_t prefetchable_memory_limit_hi;
    uint16_t io_base_hi;
    uint16_t io_limit_hi;
    uint8_t capability_pointer;
    uint8_t reserved0;
    uint16_t reserved1;
    uint32_t expansion_rom_base_address;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint16_t bridge_control;
} __attribute__((packed));
typedef struct pci_header_0x1 pci_header_0x1_t;

struct pci_header_0x2
{
    pci_header_common_t common;
    uint32_t cardbus_socket_base_address;
    uint8_t offset_to_capabilities_list;
    uint8_t reserved0;
    uint16_t secondary_status;
    uint8_t pci_bus_number;
    uint8_t cardbus_bus_number;
    uint8_t subordinate_bus_number;
    uint8_t cardbus_latency_timer;
    uint32_t memory_base_address0;
    uint32_t memory_limit0;
    uint32_t memory_base_address1;
    uint32_t memory_limit1;
    uint32_t io_base_address0;
    uint32_t io_limit0;
    uint32_t io_base_address1;
    uint32_t io_limit1;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint16_t bridge_control;
    uint16_t subsystem_device_id;
    uint16_t subsystem_vendor_id;
    uint32_t legacy_mode_base_address;
} __attribute__((packed));
typedef struct pci_header_0x2 pci_header_0x2_t;

typedef struct pci_devices_list_entry
{
    struct pci_devices_list_entry* next;
    uint64_t header_paddr;
} pci_devices_list_entry_t;

typedef struct
{
    pci_devices_list_entry_t* head;
    pci_devices_list_entry_t* tail;
} pci_devices_list_t;

typedef enum
{
    PCI_HEADER_0x0,
    PCI_HEADER_0x1,
    PCI_HEADER_0x2
} pci_header_type_t;

int pci_init(void);
pci_devices_list_t* pci_find_devices(int class, int subclass, int program_interface);
void pci_delete_devices_list(pci_devices_list_t* list);

#endif
