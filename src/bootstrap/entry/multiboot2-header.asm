[section .multiboot]
[bits 32]

%define MULTIBOOT2_MAGIC 0xE85250D6
%define MULTIBOOT2_ARCH 0
%define MULTIBOOT2_HDRLEN (multiboot2_header.end - multiboot2_header)
%define MULTIBOOT2_CHECKSUM -(MULTIBOOT2_ARCH + MULTIBOOT2_MAGIC + MULTIBOOT2_HDRLEN)
%define MULTIBOOT2_ALIGN 8

multiboot2_header:
    ; Magic values
    align MULTIBOOT2_ALIGN
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd MULTIBOOT2_HDRLEN
    dd MULTIBOOT2_CHECKSUM
    ; Framebuffer header tag
    align MULTIBOOT2_ALIGN
    dw 5 ; Type
    dw 1 ; Is optional
    dd 20 ; Size (in bytes)
    dd 1280 ; Requested width
    dd 720 ; Requested height
    dd 32 ; Requested color depth
    ; Module alignment tag
    align MULTIBOOT2_ALIGN
    dw 6 ; Type
    dw 1 ; Is optional
    dd 8 ; Size (in bytes)
    ; End tag
    align MULTIBOOT2_ALIGN
    dw 0
    dw 0
    dd 8
    .end: