ENTRY(_start)
SECTIONS
{
    . = 0x0000000000000000;

    .text :
    {
        *(.crt0)
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

    .text :
    {
        . = ALIGN(4K);
        *(.exposed)
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
    }
}