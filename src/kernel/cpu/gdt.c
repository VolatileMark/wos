#include "gdt.h"
#include "tss.h"
#include "../utils/constants.h"
#include <mem.h>

#define GDT_NUM_TSS_ENTRIES 1
#define GDT_NUM_STD_ENTRIES 5
#define GDT_NUM_ENTRIES (GDT_NUM_STD_ENTRIES + GDT_NUM_TSS_ENTRIES * 2)

struct gdt_descriptor 
{
    uint16_t size;
    uint64_t addr;
} __attribute__((packed));
typedef struct gdt_descriptor gdt_descriptor_t;

struct gdt_entry 
{
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t base_mid;
    uint8_t accessed : 1;
    uint8_t rw_enable : 1;
    uint8_t privilege_direction : 1;
    uint8_t executable : 1;
    uint8_t not_tss : 1;
    uint8_t privilege_level : 2;
    uint8_t present : 1;
    uint8_t limit_hi : 4;
    uint8_t reserved : 1;
    uint8_t long_mode : 1;
    uint8_t size : 1;
    uint8_t granularity : 1;
    uint8_t base_hi;
} __attribute__((packed));
typedef struct gdt_entry gdt_entry_t;

typedef enum 
{
    GDT_SEG_NULL,
    GDT_SEG_CODE,
    GDT_SEG_DATA
} GDT_SEG_TYPE;

__attribute__((aligned(SIZE_4KB))) gdt_entry_t gdt[GDT_NUM_ENTRIES];
__attribute__((aligned(SIZE_4KB))) gdt_descriptor_t gdt_descriptor;

static uint16_t kernel_cs, kernel_ds;
static uint16_t user_cs, user_ds;
static uint16_t tss_ss;

extern void load_gdt(uint64_t gdt_descriptor_addr, uint16_t kernel_cs, uint16_t kernel_ds);

static uint16_t create_gdt_entry
(
    uint16_t index, 
    uint32_t base, 
    uint32_t limit, 
    PRIVILEGE_LEVEL privilege_level,
    GDT_SEG_TYPE type
)
{
    gdt_entry_t* entry = &gdt[index];
    if (type == GDT_SEG_NULL)
        memset(entry, 0, sizeof(gdt_entry_t));
    else
    {
        entry->base_lo = base & 0x0000FFFF;
        entry->base_mid = (base >> 16) & 0x000000FF;
        entry->base_hi = (base >> 24) & 0x000000FF;

        entry->limit_lo = limit & 0x0000FFFF;
        entry->limit_hi = (limit >> 16) & 0x0000000F;
        
        entry->accessed = 0;
        entry->rw_enable = 1;
        entry->privilege_direction = 0;
        entry->executable = (type == GDT_SEG_CODE);
        entry->not_tss = 1;
        entry->privilege_level = privilege_level;
        entry->present = 1;

        entry->reserved = 0;
        entry->long_mode = 1;
        entry->size = 0;
        entry->granularity = 1;
    }

    return (index * sizeof(gdt_entry_t));
}

static uint16_t create_tss_entry(uint16_t index, uint64_t addr)
{
    gdt_entry_t* entry = &gdt[index];
    
    entry->base_lo = addr & 0x0000FFFF;
    entry->base_mid = (addr >> 16) & 0x000000FF;
    entry->base_hi = (addr >> 24) & 0x000000FF;

    entry->limit_lo = sizeof(tss_t) - 1;
    entry->limit_hi = 0;

    entry->accessed = 1;
    entry->rw_enable = 0;
    entry->privilege_direction = 0;
    entry->executable = 1;
    entry->not_tss = 0;
    entry->privilege_level = PL0;
    entry->present = 1;

    entry->reserved = 0;
    entry->long_mode = 0;
    entry->size = 0;
    entry->granularity = 0;

    addr >>= 32;
    ++entry;
    memset(entry, 0, sizeof(gdt_entry_t));
    entry->limit_lo = addr & 0x0000FFFF;
    entry->base_lo = (addr >> 16) & 0x0000FFFF;

    return (index * sizeof(gdt_entry_t));
}

void init_gdt(void)
{
    uint64_t tss_addr = (uint64_t) get_tss();

    create_gdt_entry(0, 0x00000000, 0x00000000, PL0, GDT_SEG_NULL);
    kernel_cs = create_gdt_entry(1, 0x00000000, 0xFFFFFFFF, PL0, GDT_SEG_CODE);
    kernel_ds = create_gdt_entry(2, 0x00000000, 0xFFFFFFFF, PL0, GDT_SEG_DATA);
    user_cs = create_gdt_entry(4, 0x00000000, 0xFFFFFFFF, PL3, GDT_SEG_CODE);
    user_ds = create_gdt_entry(3, 0x00000000, 0xFFFFFFFF, PL3, GDT_SEG_DATA);
    tss_ss = create_tss_entry(5, tss_addr);

    gdt_descriptor.size = (GDT_NUM_ENTRIES * sizeof(gdt_entry_t)) - 1;
    gdt_descriptor.addr = (uint64_t) &gdt;

    load_gdt((uint64_t) &gdt_descriptor, kernel_cs, kernel_ds);
    load_tss(tss_ss);
}

uint16_t get_kernel_cs(void)
{
    return kernel_cs;
}

uint16_t get_kernel_ds(void)
{
    return kernel_ds;
}

uint16_t get_user_cs(void)
{
    return user_cs;
}

uint16_t get_user_ds(void)
{
    return user_ds;
}

uint16_t get_tss_ss(void)
{
    return tss_ss;
}