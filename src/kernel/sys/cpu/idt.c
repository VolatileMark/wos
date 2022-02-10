#include "idt.h"
#include "isr.h"
#include "gdt.h"
#include "../chips/pic.h"
#include "../../proc/scheduler.h"
#include "../../utils/constants.h"

#define SET_ISR(index) set_idt_gate(index, (uint64_t) &isr##index, get_kernel_cs(), PL0, IDT_INTERRUPT_GATE, 1)
#define SET_IRQ(index) set_idt_gate(IRQ(index), (uint64_t) &irq##index, get_kernel_cs(), PL0, IDT_INTERRUPT_GATE, 0)
#define SET_SFT(index) set_idt_gate(index, (uint64_t) &sft##index, get_kernel_cs(), PL3, IDT_INTERRUPT_GATE, 0)

typedef enum
{
    IDT_TRAP_GATE = 0x0,
    IDT_INTERRUPT_GATE = 0x1,
} IDT_GATE_TYPE;

struct idt64_descriptor 
{
    uint16_t limit;         /* Size in bytes of the IDT - 1 */
    uint64_t base;          /* Where the IDT starts (base address) */
} __attribute__((packed));
typedef struct idt64_descriptor idt64_descriptor_t;

struct idt64_entry 
{
    uint16_t offset_lo;                 /* Offset bits 0..15 */
    uint16_t segment_selector;          /* Code segment selector */
    uint8_t ist;                        /* Interrupt stack table */
    uint8_t type : 4;
    uint8_t zero : 1;
    uint8_t privilege_level : 2;
    uint8_t present : 1;
    uint16_t offset_mid;                /* Offset bits 16..31 */
    uint32_t offset_high;               /* Offset bits 32..63 */
    uint32_t reserved;                  /* Reserved (always 0) */
} __attribute__((packed));
typedef struct idt64_entry idt64_entry_t;

static idt64_entry_t idt[IDT_NUM_ENTRIES];
static idt64_descriptor_t idt_descriptor;

extern void load_idt(uint64_t idt); /* ASM call */

static void set_idt_gate
(
    uint8_t interrupt_number, 
    uint64_t offset, 
    uint16_t segment_selector, 
    uint8_t privilege_level, 
    IDT_GATE_TYPE type,
    uint8_t present
)
{
    idt64_entry_t* entry = &idt[interrupt_number];
    entry->offset_lo = (uint16_t) (offset & 0x000000000000FFFF);
    entry->offset_mid = (uint16_t) ((offset & 0x00000000FFFF0000) >> 16);
    entry->offset_high = (uint32_t) ((offset & 0xFFFFFFFF00000000) >> 32);
    entry->reserved = 0;
    entry->segment_selector = segment_selector;
    entry->ist = 0;
    entry->type = (type ^ 0xF);
    entry->present = present;
    entry->privilege_level = privilege_level;
    entry->zero = 0;
}

void set_idt_interrupt_present(uint8_t interrupt_number, uint8_t value)
{
    idt[interrupt_number].present = value;
}

void init_idt(void)
{
    idt_descriptor.limit = (IDT_NUM_ENTRIES * sizeof(idt64_entry_t)) - 1;
    idt_descriptor.base = (uint64_t) &idt;
    
    SET_ISR(0);
    SET_ISR(1);
    SET_ISR(2);
    SET_ISR(3);
    SET_ISR(4);
    SET_ISR(5);
    SET_ISR(6);
    SET_ISR(7);
    SET_ISR(8);
    SET_ISR(9);
    SET_ISR(10);
    SET_ISR(11);
    SET_ISR(12);
    SET_ISR(13);
    SET_ISR(14);
    SET_ISR(15);
    SET_ISR(16);
    SET_ISR(17);
    SET_ISR(18);
    SET_ISR(19);
    SET_ISR(20);
    SET_ISR(21);
    SET_ISR(22);
    SET_ISR(23);
    SET_ISR(24);
    SET_ISR(25);
    SET_ISR(26);
    SET_ISR(27);
    SET_ISR(28);
    SET_ISR(29);
    SET_ISR(30);
    SET_ISR(31);

    /* Remap PIC */
    init_pic();

    SET_IRQ(0);
    SET_IRQ(1);
    SET_IRQ(2);
    SET_IRQ(3);
    SET_IRQ(4);
    SET_IRQ(5);
    SET_IRQ(6);
    SET_IRQ(7);
    SET_IRQ(8);
    SET_IRQ(9);
    SET_IRQ(10);
    SET_IRQ(11);
    SET_IRQ(12);
    SET_IRQ(13);
    SET_IRQ(14);
    SET_IRQ(15);

    SET_SFT(128); /* Yield (probably useless, but just for fun) */

    load_idt((uint64_t) &idt_descriptor);
}