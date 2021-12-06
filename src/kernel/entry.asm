[section .text]
[bits 64]

[extern kernel_main]

[global _start]
_start:
    mov rbp, stack_bottom
    mov rsp, rbp

    jmp kernel_main


[section .bss]

%define KERNEL_STACK_SIZE 16384

stack_top:
    resb KERNEL_STACK_SIZE
stack_bottom: