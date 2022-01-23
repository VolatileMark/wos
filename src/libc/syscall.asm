[section .text]
[bits 64]

[global syscall]
syscall:
    push r9
    push r8
    push rcx
    push rdx
    push rsi
    mov rsi, rsp
    o64 syscall
    add rsp, 8*5
    ret