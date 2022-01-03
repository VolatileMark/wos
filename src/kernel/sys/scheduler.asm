[section .text]
[bits 64]

[extern run_scheduler]

[global run_process_in_user_mode]
align 8
run_process_in_user_mode:
    cli
    mov rax, rdi
    
    mov rbx, [rax + 8]
    mov rcx, [rax + 16]
    mov rdx, [rax + 24]
    mov rdi, [rax + 32]
    mov rsi, [rax + 40]
    mov rbp, [rax + 48]
    mov r8, [rax + 64]
    mov r9, [rax + 72]
    mov r10, [rax + 80]
    mov r11, [rax + 88]
    mov r12, [rax + 96]
    mov r13, [rax + 104]
    mov r14, [rax + 112]
    mov r15, [rax + 120]
    
    push qword [rax + 128]
    push qword [rax + 56]
    push qword [rax + 144]
    push qword [rax + 136]
    push qword [rax + 152]

    push rcx
    mov cx, [rax + 128]
    mov ds, cx
    mov es, cx
    mov fs, cx
    mov gs, cx
    pop rcx

    mov rax, [rax + 0]

    iretq

[global run_process_in_kernel_mode]
align 8
run_process_in_kernel_mode:
    cli
    mov rax, rdi
    
    mov rbx, [rax + 8]
    mov rcx, [rax + 16]
    mov rdx, [rax + 24]
    mov rdi, [rax + 32]
    mov rsi, [rax + 40]
    mov rbp, [rax + 48]
    mov r8, [rax + 64]
    mov r9, [rax + 72]
    mov r10, [rax + 80]
    mov r11, [rax + 88]
    mov r12, [rax + 96]
    mov r13, [rax + 104]
    mov r14, [rax + 112]
    mov r15, [rax + 120]

    mov rsp, [rax + 56]
    push qword [rax + 144]
    push qword [rax + 136]
    push qword [rax + 152]

    mov rax, [rax + 0]

    iretq

[global snapshot_and_schedule]
align 8
snapshot_and_schedule:
    cli
    mov rax, rdi
    
    mov qword [rax], 0
    mov [rax + 8], rbx
    mov [rax + 16], rcx
    mov [rax + 24], rdx
    mov [rax + 32], rdi
    mov [rax + 40], rsi
    mov [rax + 48], rbp
    mov [rax + 56], rsp
    mov [rax + 64], r8
    mov [rax + 72], r9
    mov [rax + 80], r10
    mov [rax + 88], r11
    mov [rax + 96], r12
    mov [rax + 104], r13
    mov [rax + 112], r14
    mov [rax + 120], r15
    mov [rax + 128], ss
    add qword [rax + 56], 8

    pushf
    pop qword [rax + 144]

    mov [rax + 136], cs
    mov rbx, [rsp]
    mov [rax + 152], rbx

    call run_scheduler
    jmp $