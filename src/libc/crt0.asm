[section .crt0]
[bits 64]

%define SYS_exit 0

[extern main]

[global _start]
_start:
    call main
    call _exit

_exit:
    mov rdi, SYS_exit
    syscall