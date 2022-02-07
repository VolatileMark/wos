%include "entry/multiboot2_header.asm"
%include "entry/gdt.asm"
%include "entry/checks.asm"
%include "entry/paging.asm"
%include "entry/extensions.asm"

[section .text]
[bits 32]

[global _start32]
_start32:
    ; Disable interrupt
    cli
    ; Set up new stack
    mov esp, (stack_bottom - 8)
    mov ebp, esp
    ; Save multiboot2 parameters
    push 0
    push ebx
    push 0
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
    ; Check if FXSAVE and FXRSTOR instructions are available
    call check_for_ffxsr
    ; Make sure paging is disabled
    call disable_paging
    ; Initialize page tables
    call init_basic_paging
    ; Enable paging (and long mode)
    call enable_paging
    ; Tweak the GDT to work in 64bit
    call edit_gdt
    ; Flush the CPU pipeline and enter LM
    jmp CODE_SEGSEL:_start64



[bits 64]

[extern bootstrap_main]

_start64:
    ; Enable SSE
    call enable_sse
    ; Restore multiboot2 parameters
    pop rdi
    pop rsi
    ; Jump to C
    call bootstrap_main
    jmp $



[section .bss]

%define INITIAL_STACK_SIZE 16384

stack_top:
    resb INITIAL_STACK_SIZE
stack_bottom: