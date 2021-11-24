[section .text]
[bits 64]

[global get_current_pml4_paddr]
get_current_pml4_paddr:
    mov rax, cr3
    and rax, 0x00000000FFFFF000
    ret