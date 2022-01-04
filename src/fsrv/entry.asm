[section .entry]
[bits 64]

[extern main]

[global _start]
_start:
    mov rax, 1
    mov rdi, rax
    call main
    int 0x80
    push rax
    pop rbx
    jmp $