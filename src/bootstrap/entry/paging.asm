[section .bss]

%define SIZE_4KB 4096

align SIZE_4KB
pml4:
    resb SIZE_4KB

align SIZE_4KB
pdp_0:
    resb SIZE_4KB

align SIZE_4KB
pd_0:
    resb SIZE_4KB

align SIZE_4KB
pt_0:
    resb SIZE_4KB



[section .text]
[bits 32]

%define PTE_FLAGS 0x03

[extern _start_addr]

init_paging:
    ; Set CR3 to pml4
    mov edi, pml4
    mov cr3, edi
    ; Zero out pml4
    call clear_table
    ; Set entry 0 of pml4 to point to pdp_0
    mov edi, pdp_0
    call clear_table
    or edi, PTE_FLAGS
    mov [pml4], edi
    ; Set entry 0 of pdp_0 to point to pd_0
    mov edi, pd_0
    call clear_table
    or edi, PTE_FLAGS
    mov [pdp_0], edi
    ; Set entry 0 of pd_0 to point to pt_0
    mov edi, pt_0
    or edi, PTE_FLAGS
    mov [pd_0], edi
    ; Map the first 2MB of memory
    mov edi, pt_0
    mov eax, 0x00000000
    call fill_table
    ret

fill_table:
    mov ecx, 512
    or eax, PTE_FLAGS
    .fill_loop:
        mov [edi], eax
        add eax, SIZE_4KB
        add edi, 8
        loop .fill_loop
    ret

clear_table:
    push edi
    mov al, 0
    mov ecx, SIZE_4KB
    rep stosb
    pop edi
    ret

enable_paging:
    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    ; Enable Long Mode
    ; 1. Read EFER register from the MSRs
    mov ecx, 0xC0000080
    rdmsr
    ; 2. Set the 8th bit
    or eax, 1 << 8
    wrmsr
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

disable_paging:
    mov eax, cr0
    and eax, ~(1 << 31)
    mov cr0, eax
    ret