[section .text]
[bits 64]

[extern tss_get]

[global scheduler_switch_pml4_and_stack]
scheduler_switch_pml4_and_stack:
    call tss_get
    pop rcx
    pop rdx
    pop rdx
    mov rsp, [rax + 4]
    mov rbp, rsp
    mov cr3, rdi
    push 0
    push rbp
    push rdx
    push rcx
    ret
