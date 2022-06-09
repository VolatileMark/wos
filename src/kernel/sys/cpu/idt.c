#include "idt.h"
#include "isr.h"
#include "gdt.h"
#include "../chips/pic.h"
#include "../../headers/constants.h"

#define SET_ISR(index) idt_set_gate(index, (uint64_t) &isr##index, gdt_get_kernel_cs(), PL0, IDT_INTERRUPT_GATE, 1)
#define SET_IRQ(index) idt_set_gate(IRQ(index), (uint64_t) &irq##index, gdt_get_kernel_cs(), PL0, IDT_INTERRUPT_GATE, 0)
#define SET_SFT(index) idt_set_gate(index, (uint64_t) &sft##index, gdt_get_kernel_cs(), PL3, IDT_INTERRUPT_GATE, 0)

typedef enum
{
    IDT_TRAP_GATE,
    IDT_INTERRUPT_GATE,
} idt_gate_type_t;

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

extern void idt_load(uint64_t idt); /* ASM call */

static void idt_set_gate
(
    uint8_t interrupt_number, 
    uint64_t offset, 
    uint16_t segment_selector, 
    uint8_t privilege_level, 
    idt_gate_type_t type,
    uint8_t present
)
{
    idt64_entry_t* entry;
    entry = &idt[interrupt_number];
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

void idt_set_interrupt_present(uint8_t interrupt_number, uint8_t value)
{
    idt[interrupt_number].present = value;
}

void idt_init(void)
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
    pic_init();

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

    idt_load((uint64_t) &idt_descriptor);
}

idt64_descriptor_t* idt_get_descriptor(void)
{
    return &idt_descriptor;
}