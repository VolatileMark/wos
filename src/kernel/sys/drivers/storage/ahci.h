#ifndef __AHCI_H__
#define __AHCI_H__

#include <stdint.h>

typedef enum
{
    AHCI_DEV_NULL,
    AHCI_DEV_SATAPI,
    AHCI_DEV_SEMB,
    AHCI_DEV_PM,
    AHCI_DEV_SATA
} ahci_device_type_t;

#define AHCI_DEV_COORDS(c, p) ((c << 16) | (p & 0x0000FFFF))

int init_ahci_driver(void);
uint64_t ahci_read_sectors(uint32_t device_coordinates, uint64_t address, uint64_t sectors_count, void* buffer);
uint64_t ahci_read_bytes(uint32_t device_coordinates, uint64_t address, uint64_t bytes_count, void* buffer);

#endif