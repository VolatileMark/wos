[bits 64]

[section .text]


; port_out -> Send a byte to an I/O port
; args -> ESI the data byte (8 bits)
;         EDI the port address (16 bits)
[global port_out]
port_out:
    mov eax, esi ; Load the byte to be written to the register AL
    mov edx, edi ; Load the target port address to the register DX
    out dx, al ; Send the data (DX = lower 16 bits of EDX, AL = lower 8 bits of EAX)
    ret ; Return

; port_in -> Read a byte from an I/O port
; args -> EDI the data byte (8 bits)
[global port_in]
port_in:
    mov dx, di ; Load the target port address to the register DX
    in al, dx ; Read a byte from the I/O port and store it in the register AL 
    ret ; Return

[global port_wait]
port_wait:
    mov al, 0
    out 0x80, al
    ret
