[bits 64]

[section .text]



[global tss_load]
tss_load:
    mov rax, rdi
    ltr ax
    ret
