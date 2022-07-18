[bits 64]

[section .text]

[global idt_load]
idt_load:
    ; RDI contains the pointer to the IDT descriptor struct.
    lidt [rdi]
    ret
