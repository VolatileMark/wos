[bits 64]

enable_sse:
    mov rax, cr0
    mov rcx, ~(1 << 2)
    and rax, rcx
    or rax, 1 << 1
    mov cr0, rax
    mov rax, cr4
    or rax, 1 << 9
    or rax, 1 << 10
    mov cr4, rax
    ret