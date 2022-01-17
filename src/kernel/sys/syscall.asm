[section .text]
[bits 64]

[extern get_kernel_cs]
[extern get_kernel_ds]

[extern syscall_handler]
[extern get_tss]

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
    mov rbp, [rax + 4]
    mov rsp, rbp
    push rcx
    ret

restore_user_stack:
    pop rcx
    mov rbp, [user_rbp]
    mov rsp, [user_rsp]
    push rcx
    ret



[section .data]
user_rsp: dq 0
user_rbp: dq 0