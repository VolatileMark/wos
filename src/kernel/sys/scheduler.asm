[section .text]
[bits 64]

[extern run_scheduler]
[extern alignu]

[global run_process_in_user_mode]
align 8
run_process_in_user_mode:
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

[global run_process_in_kernel_mode]
align 8
run_process_in_kernel_mode:
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

    mov rsp, [rax + 8*18]
    push qword [rax + 8*17]
    push qword [rax + 8*19]
    push qword [rax + 8*15]

    mov rax, [rax + 8*0]

    iretq

[global snapshot_and_schedule]
align 8
snapshot_and_schedule:
    cli
    push rdi
    mov rax, rdi
    
    mov qword [rax + 8*0], 0
    mov [rax + 8*1], rbx
    mov [rax + 8*2], rcx
    mov [rax + 8*3], rdx
    mov [rax + 8*4], rdi
    mov [rax + 8*5], rsi
    mov [rax + 8*6], rbp
    mov [rax + 8*7], r8
    mov [rax + 8*8], r9
    mov [rax + 8*9], r10
    mov [rax + 8*10], r11
    mov [rax + 8*11], r12
    mov [rax + 8*12], r13
    mov [rax + 8*13], r14
    mov [rax + 8*14], r15

    mov [rax + 8*18], rsp
    add qword [rax + 8*18], 8

    mov [rax + 8*19], ss

    pushf
    pop qword [rax + 8*17]

    mov [rax + 8*16], cs
    mov rbx, [rsp]
    mov [rax + 8*15], rbx

    pop rdi
    add rdi, 8*20
    mov rsi, 16
    call alignu
    fxsave [rax]

    call run_scheduler
    jmp $