[bits 64]

[section .text]

[global load_idt]
load_idt:
    ; RDI contains the pointer to the IDT descriptor struct.
    lidt [rdi]
    ret