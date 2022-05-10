[section .text]
[bits 64]

[extern gdt_get_kernel_cs]
[extern gdt_get_kernel_ds]

[extern tss_get]

[extern syscall_handler]


[global syscall_init]
syscall_init:
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
    call gdt_get_kernel_ds
    or ax, 0x03
    mov dx, ax
    shl edx, 16
    call gdt_get_kernel_cs
    mov dx, ax
    wrmsr

    mov rcx, 0xC0000084 ; SFMASK register selector
    rdmsr
    mov eax, 0x00000000
    or eax, 1 << 9 ; Set SFMASK to disable interrupts on SYSCALL
    wrmsr

    ret

syscall_hook:
    ; Save return address and FLAGS
    push rcx
    push r11
    ; Save user RBP to the user stack and the user RSP to RBX
    push rbp
    mov rbx, rsp
    ; Load the kernel stack
    call tss_get
    mov rbp, [rax + 4]
    mov rsp, rbp
    ; Store syscall arguments on the stack
    push rsi
    push rdx
    push r10
    push r8
    push r9
    ; Set second argument to be the pointer to the stack
    mov rsi, rbp
    ; Call syscall handler
    call syscall_handler
    ; Restore user stack RSP and RBP
    mov rsp, rbx
    pop rbp
    ; Restore return address and FLAGS
    pop r11
    pop rcx
    o64 sysret