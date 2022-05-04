[bits 64]

[section .text]



[global interrupts_enable]
interrupts_enable:
    sti
    ret

[global interrupts_disable]
interrupts_disable:
    cli
    ret