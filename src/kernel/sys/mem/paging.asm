[section .text]
[bits 64]

[global pml4_load]
pml4_load:
    mov rax, rdi
    mov cr3, rax
    ret

[global tlb_flush]
tlb_flush:
    mov rax, cr3
    mov cr3, rax
    ret

[global paging_get_current_pml4_paddr]
paging_get_current_pml4_paddr:
    mov rax, cr3
    and rax, 0x00000000FFFFF000
    ret

[global pte_invalidate]
pte_invalidate:
    invlpg [rdi]
    ret