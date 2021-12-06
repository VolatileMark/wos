#include "idt.h"
#include "isr.h"
#include "gdt.h"
#include "../drivers/chips/pic.h"
#include "../utils/constants.h"

#define IDT_TRAP_GATE 0
#define IDT_INTERRUPT_GATE 1
#define IDT_CALL_GATE 2

#define SET_ISR(index) set_idt_gate(index, (uint64_t) &isr##index, get_kernel_cs(), PL0, IDT_INTERRUPT_GATE, 1)
#define SET_IRQ(index) set_idt_gate(32 + index, (uint64_t) &irq##index, get_kernel_cs(), PL0, IDT_INTERRUPT_GATE, 0)

struct idt64_descriptor {
    uint16_t limit;         /* Size in bytes of the IDT - 1 */
    uint64_t base;          /* Where the IDT starts (base address) */
} __attribute__((packed));
typedef struct idt64_descriptor idt64_descriptor_t;

struct idt64_entry {
    uint16_t offset_lo;                 /* Offset bits 0..15 */
    uint16_t segment_selector;          /* Code segment selector */
    uint8_t ist;                        /* Interrupt stack table */
    uint8_t flags;                      /* Type and attributes */
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
    uint8_t type,
    uint8_t is_present
)
{
    idt[interrupt_number].offset_lo = (uint16_t) (offset & 0x000000000000FFFF);
    idt[interrupt_number].offset_mid = (uint16_t) ((offset & 0x00000000FFFF0000) >> 16);
    idt[interrupt_number].offset_high = (uint32_t) ((offset & 0xFFFFFFFF00000000) >> 32);
    
    idt[interrupt_number].reserved = 0;

    idt[interrupt_number].segment_selector = segment_selector;

    idt[interrupt_number].flags = (is_present << 7) & 0b10000000;
    idt[interrupt_number].flags |= (privilege_level << 5) & 0b01100000;
    if (type != IDT_CALL_GATE)
        idt[interrupt_number].flags |= type ^ 0b00001111; /* (bit 2-3) if it's 32-bit -> 0b11, if it's 16-bit -> 0b01 */
    else
        idt[interrupt_number].flags |= 0b00000101;
    idt[interrupt_number].flags &= 0b11101111; /* Bit 4 is set to zero for interrupt and trap gates */

    idt[interrupt_number].ist = 0;
}

void set_idt_interrupt_present(uint8_t interrupt_number, uint8_t value)
{
    uint8_t mask = 0b10000000;
    idt[interrupt_number].flags &= ~mask;
    if (value)
        idt[interrupt_number].flags |= mask;
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
    
    load_idt((uint64_t) &idt_descriptor);
}