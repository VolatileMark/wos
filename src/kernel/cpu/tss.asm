[bits 64]

[section .text]



[global load_tss]
load_tss:
    mov rax, rdi
    ltr ax
    ret