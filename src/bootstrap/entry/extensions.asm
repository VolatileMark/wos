[section .text]
[bits 64]

enable_sse:
    mov rax, cr0
    ; Disable x87 FPU emulation
    mov rcx, ~(1 << 2)
    and rax, rcx
    ; Clear task switch flag
    mov rcx, ~(1 << 3)
    and rax, rcx
    ; Set monitor coprocessor flag
    or rax, 1 << 1
    mov cr0, rax

    mov rax, cr4
    ; Enable OS support for FXSAVE and FXRSTOR instructions
    or rax, 1 << 9
    ; Enable OS support for unmasked SIMD floating-point exceptions
    or rax, 1 << 10
    mov cr4, rax
    
    ; Enable FFXSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 14
    wrmsr
    ret
