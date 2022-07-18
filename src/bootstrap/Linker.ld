OUTPUT_FORMAT(elf64-x86-64)

LOAD_ADDRESS = 0x00000000; /* Load at 0MB */

ENTRY(_start32)
SECTIONS
{
    . = LOAD_ADDRESS;

    _start_addr = .;

    .text ALIGN(4K) :
    {
        KEEP(*(.multiboot2))
        *(.text)
    }

    .data ALIGN(4K) :
    {
        *(.data)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    }

    .symtab ALIGN(4K) :
    {
        *(.symtab*)
        *(.strtab*)
        *(.shstrtab*)
    }

    .bss ALIGN(4K) :
    {
        *(.bss)
        *(COMMON)
    }

    _end_addr = .;
}
