[section .text]
[bits 64]

[extern kernel_main]

[global _start]
_start:
    mov rsp, (kernel_stack_bottom - 8)
    mov rbp, rsp

    jmp kernel_main


[section .bss]

%define KERNEL_STACK_SIZE 16384

[global kernel_stack_top]
[global kernel_stack_bottom]
kernel_stack_top:
    resb KERNEL_STACK_SIZE
kernel_stack_bottom: