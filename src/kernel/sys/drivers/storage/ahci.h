#ifndef __AHCI_H__
#define __AHCI_H__

typedef enum
{
    AHCI_DEV_NULL,
    AHCI_DEV_SATAPI,
    AHCI_DEV_SEMB,
    AHCI_DEV_PM,
    AHCI_DEV_SATA
} ahci_device_type_t;

#endif