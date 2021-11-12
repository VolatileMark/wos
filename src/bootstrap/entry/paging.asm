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
%define LOAD_ADDRESS 0x00100000

[extern _start_addr]

init_paging:
    mov eax, 0
    ; Zero out pml4
    mov edi, pml4
    mov ecx, SIZE_4KB
    rep stosd
    ; Zero out pdp_0
    mov edi, pdp_0
    mov ecx, SIZE_4KB
    rep stosd
    ; Add entry 0 in the pml4 to point to pdp_0
    or edi, PTE_FLAGS
    mov [pml4], edi
    ; Zero out pd_0
    mov edi, pd_0
    mov ecx, SIZE_4KB
    rep stosd
    ; Add entry 0 in the pdp_0 to point to pd_0
    or edi, PTE_FLAGS
    mov [pdp_0], edi
    ; Zero out pt_0
    mov edi, pt_0
    mov ecx, SIZE_4KB
    rep stosd
    ; Add entry 0 in the pd_0 to point to pt_0
    or edi, PTE_FLAGS
    mov [pd_0], edi
    ; Set start map address
    mov eax, LOAD_ADDRESS
    ; Set page table (Map a max of 2MB)
    mov ebx, pt_0
    ; Set flags
    or eax, PTE_FLAGS
    ; Set number of entries
    mov ecx, 512
    ; Fill pt_0
    .fill_loop:
        mov [ebx], eax
        add eax, SIZE_4KB
        add ebx, 8
        loop .fill_loop
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
    and eax, 0x7FFFFFFF
    mov cr0, eax
    ret