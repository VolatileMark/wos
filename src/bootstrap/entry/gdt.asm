[section .data]

gdt:
    ; GDT null descriptor (yupdesign.flac)
    .null:
        dw 0x0000
        dw 0x0000
        db 0x00
        db 0b00000000
        db 0b00000000
        db 0x00
    
    ; GDT code segment selector
    .cs:
        dw 0xFFFF ; Limit Low (span all the available memory)
        dw 0x0000 ; Base Low (the base is the segment's start address in memory)
        db 0x00 ; Base Medium
        ; Access bytes
        ; Bit 7: is code segment present in memory (1 = True; 0 = False)
        ; Bit 6-5: privilege access level (00 = Kernel; 03 = User)
        ; Bit 4: descriptor type (1 = Code, Data; 0 = System)
        ; Bit 3: is executable (1 = True; 0 = False)
        ; Bit 2: conforming bit (idk)
        ; Bit 1: read or write (1 = Read; 0 = Write)
        ; Bit 0: is being accessed (must be 0, it's set by the CPU)
        db 0b10011010
        ; Flags and Limit High
        ; Bit 7: enable 4kib blocks (1 = True, 0 = False)
        ; Bit 6: is 32 bit (1 = True, 0 = False)
        ; Bit 5: is 64 bit (1 = True, 0 = False)
        ; Bit 4: available (used by software, always 0)
        ; Bit 3-0: final part of the limit
    .cs_flags: db 0b11001111
        db 0x00 ; Base High

    ; GDT data segment selector
    .ds:
        dw 0xFFFF ; Limit Low (span all the available memory)
        dw 0x0000 ; Base Low (the base is the segment's start address in memory)
        db 0x00 ; Base Medium
        ; Access bytes
        ; Bit 7: is code segment present in memory (1 = True; 0 = False)
        ; Bit 6-5: privilege access level (00 = Kernel; 03 = User)
        ; Bit 4: descriptor type (1 = Code, Data; 0 = System)
        ; Bit 3: is executable (1 = True; 0 = False)
        ; Bit 2: conforming bit (idk)
        ; Bit 1: read or write (1 = Read; 0 = Write)
        ; Bit 0: is being accessed (must be 0, it's set by the CPU)
        db 0b10010010
        ; Flags and Limit High
        ; Bit 7: enable 4kib blocks (1 = True, 0 = False)
        ; Bit 6: is 32 bit (1 = True, 0 = False)
        ; Bit 5: is 64 bit (1 = True, 0 = False)
        ; Bit 4: available (used by software, always 0)
        ; Bit 3-0: final part of the limit
    .ds_flags: db 0b11001111
        db 0x00 ; Base high
    
    .end:


gdt_descriptor:
    dw ((gdt.end - gdt.null) - 1) ; GDT Size
    dq gdt ; GDT Address



[section .text]
[bits 32]

CODE_SEGSEL equ (gdt.cs - gdt.null)
DATA_SEGSEL equ (gdt.ds - gdt.null)

load_gdt:
    mov edi, gdt_descriptor
    lgdt [edi]
    jmp CODE_SEGSEL:far_jump ; Flush the CPU pipeline and update the CS register

far_jump:
    ; Reload segment registers
    mov ax, DATA_SEGSEL
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    ret

edit_gdt:
    mov byte [gdt.cs_flags], 0b10101111
    mov byte [gdt.ds_flags], 0b10101111
    ret
