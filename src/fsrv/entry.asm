[section .entry]
[bits 64]

[extern main]

[global _start]
_start:
    mov rax, 1
    mov rdi, rax
    call main
    push rax
    pop rbx
    jmp $