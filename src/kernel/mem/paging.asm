[section .text]
[bits 64]

[global load_pml4]
load_pml4:
    mov rax, rdi
    mov cr3, rax
    ret

[global flush_tlb]
flush_tlb:
    mov rax, cr3
    mov cr3, rax
    ret

[global get_current_pml4_paddr]
get_current_pml4_paddr:
    mov rax, cr3
    and rax, 0x00000000FFFFF000
    ret

[global invalidate_pte]
invalidate_pte:
    invlpg [rdi]
    ret