[bits 64]

[section .text]


; ports_write_byte -> Send a byte to an I/O port
; args -> ESI the data byte (8 bits)
;         EDI the port address (16 bits)
[global write_port]
write_port:
    mov eax, esi ; Load the byte to be written to the register AL
    mov edx, edi ; Load the target port address to the register DX
    out dx, al ; Send the data (DX = lower 16 bits of EDX, AL = lower 8 bits of EAX)
    ret ; Return

; ports_read_byte -> Read a byte from an I/O port
; args -> EDI the data byte (8 bits)
[global read_port]
read_port:
    mov dx, di ; Load the target port address to the register DX
    in al, dx ; Read a byte from the I/O port and store it in the register AL 
    ret ; Return

[global wait_for_ports]
wait_for_ports:
    mov al, 0
    out 0x80, al
    ret
