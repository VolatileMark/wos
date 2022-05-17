[section .text]
[bits 64]

[extern kernel_stack_bottom]

[extern gdt_get_kernel_cs]
[extern gdt_get_kernel_ds]

[extern memcpy]
[extern tss_get]

[extern syscall_handler]


[global syscall_switch_to_kernel_stack]
syscall_switch_to_kernel_stack:
    ; Save return return function address to RBX
    mov rbx, rdi
    ; Load kernel stack
    mov rbp, (kernel_stack_bottom - 8)
    mov rsp, rbp
    ; Move the stack pointer to make space for memcpy copy
    sub rsp, 8*4
    ; Set memcpy's dest address to the current stack pointer
    mov rdi, rsp
    ; Move the syscall stack pointer to the last parameter
    ; NOTE: RSI is memcpy's src address
    sub rsi, 8*4
    ; Set memcpy size to (5 * sizeof(uint64_t))
    mov rdx, 8*5
    ; Start copy!
    call memcpy
    ; Load the new syscall stack pointer as parameter
    mov rdi, rbp
    ; Push return address to the stack
    push rbx
    ; Return to hook
    ret

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
    mov [rsp], rsi
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
