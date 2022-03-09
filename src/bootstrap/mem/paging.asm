[section .text]
[bits 64]

[global pml4_get_current_paddr]
pml4_get_current_paddr:
    mov rax, cr3
    and rax, 0x00000000FFFFF000
    ret

[global pte_invalidate]
pte_invalidate:
    invlpg [rdi]
    ret