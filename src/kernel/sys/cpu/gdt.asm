[section .text]
[bits 64]

[global gdt_load]
gdt_load:
    lgdt [rdi]
    mov ax, dx
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    pop rdi
    mov rax, rsi
    push rax
    push rdi
    retfq