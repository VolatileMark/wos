[section .text]
[bits 64]

[extern alignu]

[global scheduler_switch_pml4_and_stack]
scheduler_switch_pml4_and_stack:
    pop rcx
    mov rsp, rsi
    mov rbp, rsp
    mov rdx, [rdi+8*2]
    mov cr3, rdx
    push rcx
    mov rax, rdi
    ret

[global scheduler_run_process]
align 8
scheduler_run_process:
    cli

    push rdi
    add rdi, 8*20
    mov rsi, 16
    call alignu
    fxrstor [rax]
    pop rdi

    mov rax, rdi
    
    mov rbx, [rax + 8*1]
    mov rcx, [rax + 8*2]
    mov rdx, [rax + 8*3]
    mov rdi, [rax + 8*4]
    mov rsi, [rax + 8*5]
    mov rbp, [rax + 8*6]
    mov r8, [rax + 8*7]
    mov r9, [rax + 8*8]
    mov r10, [rax + 8*9]
    mov r11, [rax + 8*10]
    mov r12, [rax + 8*11]
    mov r13, [rax + 8*12]
    mov r14, [rax + 8*13]
    mov r15, [rax + 8*14]
    
    push qword [rax + 8*19]
    push qword [rax + 8*18]
    push qword [rax + 8*17]
    push qword [rax + 8*16]
    push qword [rax + 8*15]

    push rcx
    mov cx, [rax + 8*19]
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx
    pop rcx

    mov rax, [rax + 8*0]

    iretq