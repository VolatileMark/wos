[section .text]
[bits 64]

[global syscall]
syscall:
    o64 syscall
    ret