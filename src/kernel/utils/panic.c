#include "panic.h"
#include "tty.h"
#include "macros.h"
#include "../sys/cpu/gdt.h"
#include "../sys/cpu/idt.h"
#include <string.h>
#include <stddef.h>

static const char symbol[] = {
    'C', '-', 'P', '-',
    'A', '-', 'Z', 'S',
    '-', 'I', '-', 'O'
};

static uint16_t panic_bit_index(uint16_t num, uint16_t mask)
{
    uint16_t i, bit;
    for (i = 0, bit = num & mask; i < 64 && !(bit & 1); i++, bit >>= 1);
    return i;
}

static void panic_print_flags(uint16_t flags)
{
    uint16_t mask, i;
    tty_putc('[');
    for (mask = 0x0800; mask != 0; mask >>= 1)
    {
        /* Ignore reserved flags */
        if 
        (
            mask == 0x0002 || mask == 0x0008 || 
            mask == 0x0020 || mask == 0x0100 ||
            mask == 0x0400
        )
            continue;
        i = panic_bit_index(flags, mask);
        tty_putc((i < 12) ? symbol[i] : '-');
    }
    tty_putc(']');
}

static void panic_line(char filler)
{
    uint32_t i, w;
    for (i = 0, w = tty_width(); i < w; i ++)
        tty_putc(filler);
}

static void panic_title(char filler, const char* title)
{
    uint32_t i, w, l;
    l = strlen(title);
    w = (tty_width() - l) / 2;
    for (i = 0; i < w; i++)
        tty_putc(filler);
    tty_puts(title);
    for (i = 0; i < w; i++)
        tty_putc(filler);
    /* Adding the odd filler at the end is objectively prettier */
    if (l % 2 != 0)
        tty_putc(filler);
}

__attribute__((noreturn))
void panic(const interrupt_frame_t* frame, const char* msg, ...) 
{
    va_list ap;
    uint8_t cpl, dpl;
    gdt_descriptor_t* gdt_descriptor;
    idt64_descriptor_t* idt_descriptor;
    uint64_t cr0, cr2, cr3, cr4;

    if (!tty_is_initialized())
        goto END;

    tty_set_background_color(0, 0, 255);
    tty_set_foreground_color(255, 255, 0);
    tty_clear_screen();
    
    /* Print panic header */
    panic_line('#');
    panic_title('#', "  PANIC!  ");
    panic_line('#');
    tty_puts("\n");

    tty_puts(" [REASON] ");
    va_start(ap, msg);
    tty_printva(msg, ap);
    va_end(ap);
    tty_puts("\n\n");

    if (frame == NULL)
        goto END;

    panic_title('-', "  CRASH DUMP  ");
    tty_putc('\n');

    cpl = (uint8_t)(frame->stack_state.cs & 0x03);
    dpl = (uint8_t)(frame->stack_state.ss & 0x03);
    gdt_descriptor = gdt_get_descriptor();
    idt_descriptor = idt_get_descriptor();
    READ_REGISTER("cr0", cr0);
    READ_REGISTER("cr2", cr2);
    READ_REGISTER("cr3", cr3);
    READ_REGISTER("cr4", cr4);

    tty_printf
    (
        "V=%02X E=%04X CPL=%x IP=%04X:%p SP=%04X:%p\n\n", 
        frame->interrupt_info.interrupt_number, 
        frame->interrupt_info.error_code, 
        cpl, 
        frame->stack_state.cs, frame->stack_state.rip,
        frame->stack_state.ss, frame->stack_state.rsp
    );

    tty_printf
    (
        "RAX=%p RBX=%p RCX=%p RDX=%p\n", 
        frame->registers_state.rax, 
        frame->registers_state.rbx, 
        frame->registers_state.rcx, 
        frame->registers_state.rdx
    );

    tty_printf
    (
        "RSI=%p RDI=%p RBP=%p RSP=%p\n", 
        frame->registers_state.rsi, 
        frame->registers_state.rdi, 
        frame->registers_state.rbp, 
        frame->stack_state.rsp
    );

    tty_printf
    (
        "R8 =%p R9 =%p R10=%p R11=%p\n", 
        frame->registers_state.r8, 
        frame->registers_state.r9, 
        frame->registers_state.r10, 
        frame->registers_state.r11
    );

    tty_printf
    (
        "R12=%p R13=%p R14=%p R15=%p\n", 
        frame->registers_state.r12, 
        frame->registers_state.r13, 
        frame->registers_state.r14, 
        frame->registers_state.r15
    );

    tty_printf
    (
        "RIP=%p RFL=%08X ", 
        frame->stack_state.rip, 
        frame->stack_state.rflags
    );
    panic_print_flags((uint16_t) frame->stack_state.rflags);
    tty_printf(" CPL=%x\n", cpl);

    tty_printf
    (
        "CS =%04X %p %08X %08X DPL=%x\n",
        frame->stack_state.cs,
        0, 0, 0, dpl
    );

    tty_printf
    (
        "SS =%04X %p %08X %08X DPL=%x\n",
        frame->stack_state.ss,
        0, 0, 0, dpl
    );

    tty_printf
    (
        "GDT=     %p %08X\n",
        gdt_descriptor->addr,
        gdt_descriptor->size
    );
    
    tty_printf
    (
        "IDT=     %p %08X\n",
        idt_descriptor->base,
        idt_descriptor->limit
    );

    tty_printf
    (
        "CR0=%08X CR2=%p CR3=%p CR4=%08X",
        cr0, cr2, cr3, cr4
    );

END:
    HALT();
}
