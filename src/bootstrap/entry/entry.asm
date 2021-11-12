%include "entry/multiboot2_header.asm"
%include "entry/gdt.asm"
%include "entry/checks.asm"
%include "entry/paging.asm"

[section .text]
[bits 32]

[global _start]
_start:
    ; Disable interrupt
    cli
    ; Set up new stack
    mov esp, stack_top
    mov ebp, esp
    ; Save multiboot2 parameters
    push ebx
    push eax
    ; Reset FLAGS register
    push 0
    popf
    ; Load the 32bit GDT
    call load_gdt
    ; Check for CPUID
    call check_for_cpuid
    ; Check for Long Mode
    call check_for_long_mode
    ; Check for SSE
    call check_for_sse
    
    call disable_paging
    call init_paging
    call enable_paging

    jmp $



[section .bss]

%define INITIAL_STACK_SIZE 16384

stack_bottom:
    resb INITIAL_STACK_SIZE
stack_top: