#include "ahci.h"
#include "../bus/pci.h"
#include "../../../mem/pfa.h"
#include "../../../mem/paging.h"
#include "../../../utils/constants.h"
#include "../../../utils/macros.h"
#include "../../../utils/helpers/alloc.h"
#include "../../../utils/helpers/log.h"
#include <stddef.h>
#include <math.h>
#include <mem.h>

#define trace_ahci(msg, ...) trace("AHCI", msg, ##__VA_ARGS__)

#define TIMEOUT_SPIN 1000000

#define HBA_PxCMD_ST 0x0001
#define HBA_PxCMD_FRE 0x0010
#define HBA_PxCMD_FR 0x4000
#define HBA_PxCMD_CR 0x8000

#define HBA_PxIS_TFES (1 << 30)

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

#define HBA_MEM_PORTS 32
#define HBA_PORT_PRDT_ENTRIES 8

#define AHCI_ATA_DEV_BUSY 0x80
#define AHCI_ATA_DEV_DRQ 0x08

#define AHCI_ATA_CMD_READ_DMA 0xC8
#define AHCI_ATA_CMD_READ_DMA_EX 0x25
#define AHCI_ATA_CMD_WRITE_DMA 0xCA
#define AHCI_ATA_CMD_WRITE_DMA_EX 0x35
#define AHCI_ATA_CMD_MODE_LBA (1 << 6)

#define CONTROLLER_MEM(max_ports, max_cmd_slots) (((sizeof(hba_cmd_header_t) * max_cmd_slots) + sizeof(hba_fis_t) + (sizeof(hba_cmd_tbl_t) * max_cmd_slots)) * max_ports)

typedef enum
{
    SATA_DEVSIG_ATA = 0x00000101,
    SATA_DEVSIG_ATAPI = 0xEB140101,
    SATA_DEVSIG_SEMB = 0xC33C0101,
    SATA_DEVSIG_PM = 0x96690101
} sata_device_signature_t;

typedef enum
{
    FIS_TYPE_REG_H2D = 0x27,
    FIS_TYPE_REG_D2H = 0x34,
    FIS_TYPE_DMA_ACT = 0x39,
    FIS_TYPE_DMA_SETUP = 0x41,
    FIS_TYPE_DATA = 0x46,
    FIS_TYPE_BIST = 0x58,
    FIS_TYPE_PIO_SETUP = 0x5F,
    FIS_TYPE_DEV_BITS = 0xA1
} fis_type_t;

struct fis_reg_h2d
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;
    uint8_t reserved0 : 3;
    uint8_t c_bit : 1;
    uint8_t command;
    uint8_t feature_lo;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t feature_hi;
    uint16_t count;
    uint8_t icc;
    uint8_t control;
    uint32_t reserved1;
} __attribute__((packed));
typedef struct fis_reg_h2d fis_reg_h2d_t;

struct fis_reg_d2h
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;
    uint8_t reserved0 : 2;
    uint8_t interrupt_bit : 1;
    uint8_t reserved1 : 1;
    uint8_t status;
    uint8_t error;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t reserved2;
    uint16_t count;
    uint16_t reserved3;
    uint32_t reserved4;
} __attribute__((packed));
typedef struct fis_reg_d2h fis_reg_d2h_t;

struct fis_dma_setup
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;
    uint8_t reserved0 : 1;
    uint8_t d_bit : 1;
    uint8_t interrupt_bit : 1;
    uint8_t a_bit : 1;
    uint16_t reserved1;
    uint64_t dma_buffer_id;
    uint32_t reserved2;
    uint32_t dma_buffer_offset;
    uint32_t transfer_count;
    uint32_t reserved3;
} __attribute__((packed));
typedef struct fis_dma_setup fis_dma_setup_t;

struct fis_data
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;
    uint8_t reserved0 : 4;
    uint16_t reserved1;
    uint32_t data[1];
} __attribute__((packed));
typedef struct fis_data fis_data_t;

struct fis_pio_setup
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;
    uint8_t reserved0 : 1;
    uint8_t d_bit : 1;
    uint8_t interrupt_bit : 1;
    uint8_t reserved1 : 1;
    uint8_t status;
    uint8_t error;
    uint8_t lba0;
    uint8_t lba1;
    uint8_t lba2;
    uint8_t device;
    uint8_t lba3;
    uint8_t lba4;
    uint8_t lba5;
    uint8_t reserved2;
    uint16_t count;
    uint8_t reserved3;
    uint8_t new_status;
    uint16_t transfer_count;
    uint16_t reserved4;
} __attribute__((packed));
typedef struct fis_pio_setup fis_pio_setup_t;

struct fis_dev_bits
{
    uint8_t fis_type;
    uint8_t port_multiplier : 4;
    uint8_t reserved0 : 2;
    uint8_t interrupt_bit : 1;
    uint8_t n_bit : 1;
    uint8_t status_lo : 3;
    uint8_t reserved1 : 1;
    uint8_t status_hi : 3;
    uint8_t reserved2 : 1;
    uint8_t error;
    uint32_t vendor;
} __attribute__((packed));
typedef struct fis_dev_bits fis_dev_bits_t;

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
    hba_port_t ports[HBA_MEM_PORTS];
} __attribute__((packed));
typedef struct hba_mem hba_mem_t;

struct hba_fis
{
    fis_dma_setup_t dma_setup_fis;
    uint8_t pad0[4];
    fis_pio_setup_t pio_setup_fis;
    uint8_t pad1[12];
    fis_reg_d2h_t reg_d2h_fis;
    uint8_t pad2[4];
    fis_dev_bits_t dev_bits_fis;
    uint8_t u_fis[64];
    uint8_t rsv[0x100 - 0xA0];
} __attribute__((packed));
typedef struct hba_fis hba_fis_t;

long cool = sizeof(fis_dma_setup_t);

struct hba_cmd_header
{
    uint8_t cmd_fis_length : 5;
    uint8_t atapi : 1;
    uint8_t write : 1;
    uint8_t prefetchable : 1;
    uint8_t reset : 1;
    uint8_t bist : 1;
    uint8_t clear_busy : 1;
    uint8_t reserved0 : 1;
    uint8_t port_multiplier : 4;
    uint16_t prdt_length;
    uint32_t prdt_count;
    uint64_t cmd_table_base_addr;
    uint32_t reserved1[4];
} __attribute__((packed));
typedef struct hba_cmd_header hba_cmd_header_t;

struct hba_prdt_entry
{
    uint64_t data_base_address;
    uint32_t reserved0;
    uint32_t data_byte_count : 22;
    uint32_t reserved1 : 9;
    uint32_t interrupt_completion : 1;
} __attribute__((packed));
typedef struct hba_prdt_entry hba_prdt_entry_t;

struct hba_cmd_tbl
{
    uint8_t command_fis[64];
    uint8_t atapi_command[16];
    uint8_t reserved[48];
    hba_prdt_entry_t prdt_entry[HBA_PORT_PRDT_ENTRIES];
} __attribute__((packed));
typedef struct hba_cmd_tbl hba_cmd_tbl_t;

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

static ahci_controllers_list_t ahci_controllers;



static pci_header_common_t* map_pci_header(uint64_t header_paddr)
{
    uint64_t vaddr;
    vaddr = kernel_map_temporary_page(header_paddr, PAGE_ACCESS_RO, PL0);
    kernel_map_temporary_page(header_paddr + SIZE_4KB, PAGE_ACCESS_RO, PL0);
    return (pci_header_common_t*)(vaddr + GET_ADDR_OFFSET(header_paddr));
}

static void unmap_pci_header(uint64_t header_vaddr)
{
    header_vaddr = alignd(header_vaddr, SIZE_4KB);
    kernel_unmap_temporary_page(header_vaddr);
    kernel_unmap_temporary_page(header_vaddr + SIZE_4KB);
}

static ahci_device_type_t get_port_type(hba_port_t* port)
{
    uint8_t ipm, det;

    ipm = (port->ssts >> 8) & 0x0F;
    det = (port->ssts) & 0x0F;
    if 
    (
        ipm != HBA_PORT_IPM_ACTIVE || 
        det != HBA_PORT_DET_PRESENT
    )
        return AHCI_DEV_NULL;

    switch (port->sig)
    {
    case SATA_DEVSIG_ATAPI:
        return AHCI_DEV_SATAPI;
    case SATA_DEVSIG_SEMB:
        return AHCI_DEV_SEMB;
    case SATA_DEVSIG_PM:
        return AHCI_DEV_PM;
    case SATA_DEVSIG_ATA:
    default:
        return AHCI_DEV_SATA;
    }
}

static hba_mem_t* map_abar(uint64_t abar_paddr)
{
    uint64_t abar_vaddr;

    if (kernel_get_next_vaddr(sizeof(hba_mem_t), &abar_vaddr) < sizeof(hba_mem_t))
        return NULL;
    if 
    (
        kernel_map_memory(abar_paddr, abar_vaddr, sizeof(hba_mem_t), PAGE_ACCESS_RW, PL0) < sizeof(hba_mem_t) ||
        kernel_flag_memory_area(abar_vaddr, sizeof(hba_mem_t), PAGE_FLAG_UNCACHABLE)
    )
    {
        kernel_unmap_memory(abar_vaddr, sizeof(hba_mem_t));
        return NULL;
    }

    return (hba_mem_t*) abar_vaddr;
}

static void start_command_engine(hba_port_t* port)
{
    while (port->cmd & HBA_PxCMD_CR);
    port->cmd |= HBA_PxCMD_FRE | HBA_PxCMD_ST;
}

static void stop_command_engine(hba_port_t* port)
{
    port->cmd &= ~(HBA_PxCMD_FRE | HBA_PxCMD_ST);
    while (port->cmd & (HBA_PxCMD_CR | HBA_PxCMD_FR));
}

static void init_ahci_controller_sata_port
(
    uint64_t controller_base_paddr, 
    uint64_t controller_base_vaddr,
    hba_port_t* port, 
    uint64_t port_num, 
    uint8_t max_ports, 
    uint8_t max_cmd_slots,
    ahci_controller_port_descriptor_t* desc
)
{
    hba_cmd_header_t* cmd_header;
    uint8_t i;
    uint64_t cmd_headers_total_size;
    uint64_t clb_offset, fb_offset, ctb_offset;

    stop_command_engine(port);

    cmd_headers_total_size = sizeof(hba_cmd_header_t) * max_cmd_slots;
    clb_offset = (port_num * cmd_headers_total_size);
    port->clb = controller_base_paddr + clb_offset;
    desc->clb_vaddr = controller_base_vaddr + clb_offset;
    memset((void*) desc->clb_vaddr, 0, cmd_headers_total_size);

    fb_offset = (cmd_headers_total_size * max_ports) + (sizeof(hba_fis_t) * port_num);
    port->fb = controller_base_paddr + fb_offset;
    desc->fb_vaddr = controller_base_vaddr + fb_offset;
    memset((void*) desc->fb_vaddr, 0, sizeof(hba_fis_t));

    cmd_header = (hba_cmd_header_t*) (controller_base_vaddr + clb_offset);
    ctb_offset = ((cmd_headers_total_size + sizeof(hba_fis_t)) * max_ports);
    desc->ctb_vaddr = controller_base_vaddr + ctb_offset;
    for (i = 0; i < max_cmd_slots; i++)
    {
        cmd_header[i].prdt_length = HBA_PORT_PRDT_ENTRIES;
        cmd_header[i].cmd_table_base_addr = controller_base_paddr + ctb_offset + (sizeof(hba_cmd_tbl_t) * i);
        memset((void*) (desc->ctb_vaddr + (sizeof(hba_cmd_tbl_t) * i)), 0, sizeof(hba_cmd_tbl_t));
    }

    start_command_engine(port);
}

static void init_ahci_controller_ports(hba_mem_t* abar, pci_header_0x0_t* pci_header, ahci_controllers_list_entry_t* entry)
{
    hba_port_t* port;
    uint64_t controller_mem_size;
    uint32_t pi, i;
    uint8_t max_ports, ports, max_cmd_slots;
    ahci_device_type_t type;
    
    pi = abar->pi;
    max_ports = abar->cap.np + 1;
    max_cmd_slots = abar->cap.ncs + 1;
    ports = 0;

    controller_mem_size = CONTROLLER_MEM(max_ports, max_cmd_slots);
    if (kernel_get_next_vaddr(controller_mem_size, &entry->base_vaddr) < controller_mem_size)
        return;

    entry->pages = ceil((double) controller_mem_size / SIZE_4KB);
    entry->base_paddr = request_pages(entry->pages);
    if (entry->base_paddr == 0)
        return;

    if (kernel_map_memory(entry->base_paddr, entry->base_vaddr, controller_mem_size, PAGE_ACCESS_RW, PL0) < controller_mem_size)
    {
        free_pages(entry->base_paddr, entry->pages);
        return;
    }

    entry->ports = calloc(max_ports, sizeof(ahci_controller_port_descriptor_t));

    for (i = 0; i < 32 && ports < max_ports; i++)
    {
        if (pi & 1)
        {
            port = &abar->ports[i];
            type = get_port_type(port);
            switch (type)
            {
            case AHCI_DEV_NULL:
                break;
            case AHCI_DEV_SEMB:
                break;
            case AHCI_DEV_PM:
                break;
            case AHCI_DEV_SATAPI:
            case AHCI_DEV_SATA:
                init_ahci_controller_sata_port
                (
                    entry->base_paddr, 
                    entry->base_vaddr, 
                    port, 
                    ports, 
                    max_ports, 
                    max_cmd_slots, 
                    &entry->ports[ports]
                );
                break;
            }
            entry->ports[ports].type = type;
            entry->ports[ports].index = i;
            entry->ports[ports].port = port;
            ++ports;
        }
        pi >>= 1;
    }

    entry->max_ports = max_ports;
}

static uint64_t init_ahci_controller(pci_header_0x0_t* pci_header)
{
    hba_mem_t* abar;
    ahci_controllers_list_entry_t* entry;

    abar = map_abar(pci_header->bar5);
    if (abar == NULL)
    {
        trace_ahci("Could not map ABAR");
        return 0;
    }

    /* Reset the controller */
    abar->ghc.hr = 1;
    while (abar->ghc.hr != 0);

    /* Enable AHCI mode */
    if (!abar->cap.sam)
        abar->ghc.ae = 1;

    /* Check if AHCI mode was enabled */
    if (!abar->ghc.ae)
    {
        trace_ahci("AHCI is not supported by controller %x:%x", pci_header->common.vendor_id, pci_header->common.device_id);
        kernel_unmap_memory((uint64_t) abar, sizeof(hba_mem_t));
        return 0;
    }

    /* Check if 64-bit addressing is supported */
    if (!(abar->cap.s64a))
    {
        trace_ahci("64-bit addressing not supported by controller %x:%x", pci_header->common.vendor_id, pci_header->common.device_id);
        kernel_unmap_memory((uint64_t) abar, sizeof(hba_mem_t));
        return 0;
    }

    entry = malloc(sizeof(ahci_controllers_list_entry_t));
    if (entry == NULL)
    {
        trace_ahci("Could not allocate entry for controller %x:%x", pci_header->common.vendor_id, pci_header->common.device_id);
        kernel_unmap_memory((uint64_t) abar, sizeof(hba_mem_t));
        return 0;
    }

    init_ahci_controller_ports(abar, pci_header, entry);

    entry->next = NULL;
    entry->abar = abar;

    if (ahci_controllers.tail == NULL)
        ahci_controllers.head = entry;
    else
        ahci_controllers.tail->next = entry;
    ahci_controllers.tail = entry;

    return 1;
}

int init_ahci_driver(void)
{
    pci_devices_list_t* controllers;
    pci_devices_list_entry_t* controller;
    pci_header_common_t* pci_header;
    uint64_t controllers_online, i;

    memset(&ahci_controllers, 0, sizeof(ahci_controllers_list_t));

    controllers = find_pci_devices(0x1, 0x6, -1);
    controller = controllers->head;
    controllers_online = 0;
    while (controller != NULL)
    {
        pci_header = map_pci_header(controller->header_paddr);
        if 
        (
            pci_header->header_type == PCI_HEADER_0x0 && 
            init_ahci_controller((pci_header_0x0_t*) pci_header)
        )
        {
            info("AHCI controller %x:%x initialized. New ID is %d", pci_header->vendor_id, pci_header->device_id, controllers_online);
            for (i = 0; i < ahci_controllers.tail->max_ports; i++)
                info("Port %d (type %d) is implemented for controller %d", ahci_controllers.tail->ports[i].index, ahci_controllers.tail->ports[i].type, controllers_online);
            ++controllers_online;
        }
        unmap_pci_header((uint64_t) pci_header);
        controller = controller->next;
    }

    ahci_controllers.num_of_controllers = controllers_online;

    return !(controllers_online > 0);
}

static ahci_controller_port_descriptor_t* get_port(uint32_t coordinates)
{
    ahci_controllers_list_entry_t* entry;
    uint32_t controller, port, i;

    i = 0;
    port = coordinates & 0x0000FFFF;
    controller = coordinates >> 16;

    if (ahci_controllers.num_of_controllers < controller)
        return NULL;
    
    entry = ahci_controllers.head;
    while (i++ < controller)
    {
        if (entry == NULL)
            return NULL;
        entry = entry->next;
    }
    for (i = 0; i < entry->max_ports; i++)
    {
        if (entry->ports[i].index == port)
            return &entry->ports[i];
    }
    return NULL;
}

static int find_available_cmd_slot(hba_port_t* port)
{
    uint32_t slots;
    int i;

    slots = (port->sact | port->ci);
    for (i = 0; i < 32; i++)
    {
        if (!(slots & 1))
            return i;
        slots >>= 1;
    }

    return -1;
}

static uint64_t ahci_read_bytes_capped(uint32_t device_coordinates, uint64_t address, uint64_t bytes, void* buffer)
{
    ahci_controller_port_descriptor_t* desc;
    hba_cmd_header_t* cmd_header;
    hba_cmd_tbl_t* cmd_tbl;
    fis_reg_h2d_t* cmd_fis;
    uint64_t spin, i, count, buffer_addr;
    int slot;

    if (bytes == 0)
        return 0;
    bytes = minu(bytes, SIZE_nMB(4) * HBA_PORT_PRDT_ENTRIES);

    desc = get_port(device_coordinates);
    if (desc == NULL)
        return 0;
    
    desc->port->is = (uint32_t) -1;
    slot = find_available_cmd_slot(desc->port);
    if (slot == -1)
    {
        trace_ahci("No command slot available for port %x", device_coordinates);
        return 0;
    }
    
    cmd_header = (hba_cmd_header_t*) desc->clb_vaddr;
    cmd_header += slot;
    cmd_header->cmd_fis_length = (uint8_t) (sizeof(fis_reg_h2d_t) / sizeof(uint32_t));
    cmd_header->write = 0;
    cmd_header->prdt_length = (uint16_t) ceil((double) bytes / SIZE_nMB(4));
    
    cmd_tbl = (hba_cmd_tbl_t*) desc->ctb_vaddr;
    cmd_tbl += slot;
    memset(cmd_tbl, 0, sizeof(hba_cmd_tbl_t));

    buffer_addr = (uint64_t) buffer;
    count = bytes;
    for (i = 0; i < cmd_header->prdt_length - 1; i++)
    {
        cmd_tbl->prdt_entry[i].data_base_address = kernel_get_paddr(buffer_addr);
        cmd_tbl->prdt_entry[i].data_byte_count = SIZE_nMB(4) - 1;
        cmd_tbl->prdt_entry[i].interrupt_completion = 1;
        buffer_addr += SIZE_nMB(4);
        count -= SIZE_nMB(4);
    }
    cmd_tbl->prdt_entry[i].data_base_address = kernel_get_paddr(buffer_addr);
    cmd_tbl->prdt_entry[i].data_byte_count = count - 1;
    cmd_tbl->prdt_entry[i].interrupt_completion = 1;

    cmd_fis = (fis_reg_h2d_t*) &cmd_tbl->command_fis;
    cmd_fis->fis_type = FIS_TYPE_REG_H2D;
    cmd_fis->c_bit = 1;
    cmd_fis->command = AHCI_ATA_CMD_READ_DMA_EX;
    
    cmd_fis->lba0 = (uint8_t) (address >> 0x00);
    cmd_fis->lba1 = (uint8_t) (address >> 0x08);
    cmd_fis->lba2 = (uint8_t) (address >> 0x10);    
    cmd_fis->lba3 = (uint8_t) (address >> 0x18);
    cmd_fis->lba4 = (uint8_t) (address >> 0x20);
    cmd_fis->lba5 = (uint8_t) (address >> 0x28);

    cmd_fis->device = AHCI_ATA_CMD_MODE_LBA;
    cmd_fis->count = ceil((double) bytes / 512);

    for 
    (
        spin = 0; 
        (desc->port->tfd & (AHCI_ATA_DEV_BUSY | AHCI_ATA_DEV_DRQ)) && (spin < TIMEOUT_SPIN); 
        spin++
    );
    if (spin == TIMEOUT_SPIN)
    {
        trace_ahci("Port %x is hung", device_coordinates);
        return 0;
    }

    desc->port->ci = 1 << slot;

    while (1)
    {
        if (desc->port->is & HBA_PxIS_TFES)
        {
            trace_ahci("Disk read error (port %x)", device_coordinates);
            return 0;
        }
        else if (!(desc->port->ci & (1 << slot)))
            break;
    }

    return bytes;
}

uint64_t ahci_read_sectors(uint32_t device_coordinates, uint64_t address, uint64_t sectors_count, void* buffer)
{
    return ahci_read_bytes
    (
        device_coordinates,
        address, 
        sectors_count << 9, 
        buffer
    );
}

uint64_t ahci_read_bytes(uint32_t device_coordinates, uint64_t address, uint64_t bytes_count, void* buffer)
{
    uint64_t bytes_read, bytes_read_now;

    bytes_read = 0;
    while (bytes_count > 0)
    {
        bytes_read_now = ahci_read_bytes_capped(device_coordinates, address, bytes_count, buffer);
        if (bytes_read_now == 0)
            return bytes_read;
        bytes_read += bytes_read_now;
        bytes_count -= bytes_read_now;
    }
    return bytes_read;
}