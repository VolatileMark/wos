[bits 64]

[section .data]

int_frame:
    int_info:
        .int_num: dq 0
        .err_code: dq 0
    cpu_state:
        .rdi: dq 0
        .rsi: dq 0
        .rbp: dq 0
        .rdx: dq 0
        .rcx: dq 0
        .rbx: dq 0
        .rax: dq 0
        .rsp: dq 0
        .r8: dq 0
        .r9: dq 0
        .r10: dq 0
        .r11: dq 0
        .r12: dq 0
        .r13: dq 0
        .r14: dq 0
        .r15: dq 0
    stack_state:
        .rip: dq 0
        .cs: dq 0
        .rflags: dq 0
        .rsp_return: dq 0
        .ss_return: dq 0



[section .text]

%macro PUSHALL 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro POPALL 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

%macro ISR_NOERR 1
    [global isr%1]
    isr%1:
        push qword 0
        push qword %1
        jmp common_stub
%endmacro

%macro ISR_ERR 1
    [global isr%1]
    isr%1:
        push qword %1
        jmp common_stub
%endmacro

%macro IRQ 1
    [global irq%1]
    irq%1:
        push qword %1
        push qword %1+32
        jmp common_stub
%endmacro

%macro SOFTWARE 1
    [global sft%1]
    sft%1:
        push qword 0x69
        push qword %1
        jmp common_stub
%endmacro

%macro FILL_STRUCT 0
    mov [cpu_state.rdi], rdi
    mov [cpu_state.rsi], rsi
    mov [cpu_state.rbp], rbp
    mov [cpu_state.rdx], rdx
    mov [cpu_state.rcx], rcx
    mov [cpu_state.rbx], rbx
    mov [cpu_state.rax], rax
    mov [cpu_state.rsp], rsp
    mov [cpu_state.r8], r8
    mov [cpu_state.r9], r9
    mov [cpu_state.r10], r10
    mov [cpu_state.r11], r11
    mov [cpu_state.r12], r12
    mov [cpu_state.r13], r13
    mov [cpu_state.r14], r14
    mov [cpu_state.r15], r15

    mov rax, [rsp+8*0]
    mov [int_info.int_num], rax
    mov rax, [rsp+8*1]
    mov [int_info.err_code], rax

    mov rax, [rsp+8*2]
    mov [stack_state.rip], rax
    mov rax, [rsp+8*3]
    mov [stack_state.cs], rax
    mov rax, [rsp+8*4]
    mov [stack_state.rflags], rax
    mov rax, [rsp+8*5]
    mov [stack_state.rsp_return], rax
    mov rax, [rsp+8*6]
    mov [stack_state.ss_return], rax
%endmacro



[extern get_kernel_ds]
[extern isr_handler]

common_stub:
    FILL_STRUCT
    PUSHALL
    mov ax, ds
    push rax
    call get_kernel_ds
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov rdi, int_frame
    cld
    call isr_handler
    pop rbx
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    POPALL
    add rsp, 0x10
    iretq



ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR 8
ISR_NOERR 9
ISR_ERR 10
ISR_ERR 11
ISR_ERR 12
ISR_ERR 13
ISR_ERR 14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

SOFTWARE 128