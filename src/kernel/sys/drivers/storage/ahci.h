#ifndef __AHCI_H__
#define __AHCI_H__

#include <stdint.h>

#define AHCI_SECTOR_SIZE 512
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
typedef struct hba_port hba_port_t;

struct hba_mem_cap
{
    uint32_t np : 5;
    uint32_t sxs: 1;
    uint32_t ems : 1;
    uint32_t cccs : 1;
    uint32_t ncs : 5;
    uint32_t psc : 1;
    uint32_t ssc : 1;
    uint32_t pmd : 1;
    uint32_t fbss : 1;
    uint32_t spm : 1;
    uint32_t sam : 1;
    uint32_t rsv : 1;
    uint32_t iss : 4;
    uint32_t sclo : 1;
    uint32_t sal : 1;
    uint32_t salp : 1;
    uint32_t sss : 1;
    uint32_t smps : 1;
    uint32_t ssntf : 1;
    uint32_t sncq : 1;
    uint32_t s64a : 1;
} __attribute__((packed));
typedef struct hba_mem_cap hba_mem_cap_t;

struct hba_mem_ghc
{
    uint32_t hr : 1;
    uint32_t ie : 1;
    uint32_t mrsm : 1;
    uint32_t rsv : 28;
    uint32_t ae : 1;
} __attribute__((packed));
typedef struct hba_mem_ghc hba_mem_ghc_t;

struct hba_mem
{
    hba_mem_cap_t cap;
    hba_mem_ghc_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t vs;
    uint32_t ccc_ctl;
    uint32_t ccc_pts;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t ext_cap;
    uint32_t bohc;
    uint8_t reserved[0xA0 - 0x2C];
    uint8_t vendor[0x100 - 0xA0];
    hba_port_t ports[AHCI_HBA_MEM_PORTS];
} __attribute__((packed));
typedef struct hba_mem hba_mem_t;

typedef struct
{
    ahci_device_type_t type;
    uint32_t index;
    uint64_t clb_vaddr;
    uint64_t fb_vaddr;
    uint64_t ctb_vaddr;
    hba_port_t* port;
} ahci_controller_port_descriptor_t;

typedef struct ahci_controllers_list_entry
{
    struct ahci_controllers_list_entry* next;
    hba_mem_t* abar;
    uint64_t base_paddr;
    uint64_t base_vaddr;
    uint64_t pages;
    uint64_t max_ports;
    ahci_controller_port_descriptor_t* ports;
} ahci_controllers_list_entry_t;

typedef struct
{
    uint64_t num_of_controllers;
    ahci_controllers_list_entry_t* head;
    ahci_controllers_list_entry_t* tail;
} ahci_controllers_list_t;

int ahci_init(void);
uint64_t ahci_read_bytes(ahci_controller_port_descriptor_t* desc, uint64_t lba, uint64_t bytes, void* buffer);
ahci_controllers_list_t* ahci_get_controller_list(void);

#endif