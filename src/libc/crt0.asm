[section .crt0]
[bits 64]

%define SYS_exit 0
%define HEAP_INIT_SIZE 0x1000
%define HEAP_START_VADDR 0xFFFF800000000000
%define HEAP_CEIL_VADDR 0xFFFFc00000000000

[extern main]
[extern init_heap]

[global _start]
_start:
    call _init
    call main
    call _exit

_init:
    mov rdi, 0
    mov rsi, HEAP_START_VADDR
    mov rdx, HEAP_CEIL_VADDR
    mov rcx, HEAP_INIT_SIZE
    mov ax, ds
    and rax, 0x0000000000000003
    mov r8, rax
    call init_heap
    cmp rax, 0
    je .success
    sub rax, 800
    call _exit
    .success:
        ret

_exit:
    mov rdi, SYS_exit
    mov rsi, rax
    o64 syscall