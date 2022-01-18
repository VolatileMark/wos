[section .text]
[bits 32]

check_for_cpuid:
    pushfd    
    ; Store content of EFLAGS register in EAX
    pushfd
    pop eax
    ; Flip the 21st bit
    xor eax, 1 << 21
    ; Set EFLAGS to EAX
    push eax
    popfd
    ; Get the content of EFLAGS once again (this time it gets stored in EBX)
    pushfd
    pop ebx
    ; Check if the value didn't change
    xor eax, ebx
    cmp eax, 0
    ; If the value changed, fail
    jne .fail
    ; If it didn't restore the original value of EFLAGS and return
    popfd
    ret
    ; Get fucked pt. 1
    .fail:
        jmp $

check_for_long_mode:
    ; Get the largest extended function's number supported
    mov eax, 0x80000000
    cpuid
    ; Check if it's below 0x80000001
    cmp eax, 0x80000001
    ; If it is, fail
    jb .fail
    ; Otherwise call CPUID extended function number 0x80000001
    mov eax, 0x80000001
    cpuid
    ; Check if the 29th bit of EDX is 1
    test edx, 1 << 29
    ; If it's not fail
    jz .fail
    ; Otherwise return
    ret
    ; Get fucked pt. 2
    .fail:
        jmp $

check_for_sse:
    mov eax, 0x1
    cpuid
    test edx, 1 << 25
    jz .fail
    test edx, 1 << 26
    jz .fail
    ret
    ; Get fucked pt. 3
    .fail:
        jmp $

check_for_ffxsr:
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 24
    jz .fail
    ret
    ; Get fucked pt. 4
    .fail:
        jmp $