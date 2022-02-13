OUTPUT_FORMAT(elf64-x86-64)

KERNEL_BASE = 0xFFFFFFFF80000000;
KERNEL_OFFSET = 0x0000000000200000;

ENTRY(_start)
SECTIONS
{
    . = KERNEL_BASE + KERNEL_OFFSET;

    _start_addr = .;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_BASE)
    {
        *(.text)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_BASE)
    {
        *(.data)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_BASE)
    {
        *(.rodata*)
    }

    .symtab ALIGN(4K) : AT(ADDR(.symtab) - KERNEL_BASE)
    {
        *(.symtab*)
        *(.strtab*)
        *(.shstrtab*)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_BASE)
    {
        *(.bss)
        *(COMMON)
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
    }

    _end_addr = .;
}