#ifndef __AHCI_H__
#define __AHCI_H__

#include <stdint.h>

#define AHCI_HBA_MEM_PORTS 32

typedef enum
{
    AHCI_DEV_NULL,
    AHCI_DEV_SATAPI,
    AHCI_DEV_SEMB,
    AHCI_DEV_PM,
    AHCI_DEV_SATA
} ahci_device_type_t;

struct hba_port
{
    uint64_t clb;
    uint64_t fb;
    uint32_t is;
    uint32_t ie;
    uint32_t cmd;
    uint32_t rsv0;
    uint32_t tfd;
    uint32_t sig;
    uint32_t ssts;
    uint32_t sctl;
    uint32_t serr;
    uint32_t sact;
    uint32_t ci;
    uint32_t sntf;
    uint32_t fbs;
    uint32_t rsv1[11];
    uint32_t vendor[4];
} __attribute__((packed));
typedef volatile struct hba_port hba_port_t;

typedef struct
{
    ahci_device_type_t type;
    uint64_t index;
    uint64_t clb_vaddr;
    uint64_t fb_vaddr;
    uint64_t ctb_vaddr;
    hba_port_t* port;
} ahci_port_descriptor_t;

int ahci_init(void);

#endif