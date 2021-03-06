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
    mov ecx, 0xC0000082 ; LSTAR register selector
    wrmsr
    
    mov ecx, 0xC0000080 ; EFER register selector
    rdmsr
    or eax, 1 << 0 ; Enable SYSCALL and SYSRET
    wrmsr

    ; Set the correct segment selectors
    call gdt_get_kernel_ds
    or ax, 0x03
    mov cx, ax
    shl ecx, 16
    call gdt_get_kernel_cs ; Hello my friend would you happen to know another function that fucks with RDX WHEN IT SHOULDN'T?
    mov cx, ax
    mov eax, 0
    mov edx, ecx
    mov ecx, 0xC0000081 ; STAR register selector
    wrmsr

    mov ecx, 0xC0000084 ; SFMASK register selector
    mov eax, 1 << 9 ; Set SFMASK to disable interrupts on SYSCALL
    wrmsr

    ret

syscall_hook:
    ; Save return address and FLAGS
    push rcx
    push r11
    mov rcx, rdx
    ; Save user RBP to the user stack and the user RSP to RBX
    push rbp
    mov rbx, rsp
    ; Load the kernel stack
    call tss_get ; STOP FUCKING WITH RDX WHEN YOU'RE NOT SUPPOSED TO PIECE OF SHIT FUNCTION
    mov rbp, [rax + 4]
    mov rsp, rbp
    ; Store syscall arguments on the stack
    mov [rsp], rsi
    push rcx
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
