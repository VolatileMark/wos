ENTRY(_start)
SECTIONS
{
    . = 0x0000000000000000;

    .text :
    {
        *(.entry)
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .rodata :
    {
        *(.rodata*)
    }

    .symtab :
    {
        *(.symtab*)
        *(.strtab*)
        *(.shstrtab*)
    }

    .bss : 
    {
        *(.bss)
        *(COMMON)
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
    }
}