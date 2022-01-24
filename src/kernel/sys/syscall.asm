[section .text]
[bits 64]

[extern get_kernel_cs]
[extern get_kernel_ds]

[extern syscall_handler]
[extern get_tss]

[extern alignu]
[extern get_kernel_pml4_paddr]
[extern kernel_stack_bottom]


[global init_syscalls]
init_syscalls:
    ; Set LSTAR register to syscall kernel entry point
    mov rdx, syscall_hook
    shr rdx, 32
    mov rax, syscall_hook
    mov rcx, 0xC0000082 ; LSTAR register selector
    wrmsr
    
    mov rcx, 0xC0000080 ; EFER register selector
    rdmsr
    or eax, 1 << 0 ; Enable SYSCALL and SYSRET
    wrmsr

    mov rcx, 0xC0000081 ; STAR register selector
    rdmsr
    ; Set the correct segment selectors
    mov edx, 0x00000000
    call get_kernel_ds
    or ax, 0x03
    mov dx, ax
    shl edx, 16
    call get_kernel_cs
    mov dx, ax
    wrmsr

    mov rcx, 0xC0000084 ; SFMASK register selector
    rdmsr
    mov eax, 0x00000000
    or eax, 1 << 9 ; Set SFMASK to disable interrupts on SYSCALL
    wrmsr

    ret

syscall_hook:
    push rcx
    push r11
    call load_kernel_stack
    call syscall_handler
    call restore_user_stack
    pop r11
    pop rcx
    o64 sysret

load_kernel_stack:
    pop rcx
    mov [user_rsp], rsp
    mov [user_rbp], rbp
    call get_tss
    mov rsp, [rax + 4]
    mov rbp, rsp
    push rcx
    ret

restore_user_stack:
    pop rcx
    mov rbp, [user_rbp]
    mov rsp, [user_rsp]
    push rcx
    ret

[global switch_to_kernel]
switch_to_kernel:
    ; Save registers
    mov [rdi + 8*0], rax
    mov [rdi + 8*1], rbx
    mov [rdi + 8*2], rcx
    mov [rdi + 8*3], rdx
    mov [rdi + 8*4], rdi
    mov [rdi + 8*5], rsi
    mov [rdi + 8*6], rbp
    mov [rdi + 8*7], r8
    mov [rdi + 8*8], r9
    mov [rdi + 8*9], r10
    mov [rdi + 8*10], r11
    mov [rdi + 8*11], r12
    mov [rdi + 8*12], r13
    mov [rdi + 8*13], r14
    mov [rdi + 8*14], r15
    ; Save CS, RSP, SS
    mov [rdi + 8*16], cs
    mov [rdi + 8*18], rsp
    mov [rdi + 8*19], ss
    ; Store FLAGS register
    pushf
    pop qword [rdi + 8*17]
    ; Save FPU state
    push rdi
    add rdi, 8*20
    mov rsi, 16
    call alignu
    fxsave [rax]
    pop rdi

    ; Load correct data registers
    call get_kernel_ds
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Load default kernel stack and pml4
    pop rcx
    mov [rdi + 8*15], rcx ; Save return RIP
    call get_tss
    mov rdi, rax
    call get_kernel_pml4_paddr
    mov qword [rdi + 4], (kernel_stack_bottom - 8)
    mov rsp, [rdi + 4]
    mov rbp, rsp
    mov cr3, rax
    push rcx

    ret




[section .data]
user_rsp: dq 0
user_rbp: dq 0