; Multiboot header for GRUB
MULTIBOOT_PAGE_ALIGN    equ 1<<0
MULTIBOOT_MEMORY_INFO   equ 1<<1
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM      equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

section .multiboot
align 4
    dd 0x1BADB002    ; MULTIBOOT_HEADER_MAGIC
    dd 0x3           ; MULTIBOOT_HEADER_FLAGS (e.g., align modules and provide memory info)
    dd -(0x1BADB002 + 0x3)  ; MULTIBOOT_CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KB
stack_top:

section .data
debug_msg: db "Hextrix booting...", 0x0D, 0x0A, 0

section .text
global _start
extern kernel_main

; Serial port output routine
serial_out:
    mov dx, 0x3F8    ; COM1 port
    out dx, al       ; Send the byte
    ret

_start:
    ; Early debug output
    mov esi, debug_msg
.debug_loop:
    lodsb
    test al, al
    jz .debug_done
    call serial_out
    jmp .debug_loop
.debug_done:
    
    ; Setup stack
    mov esp, stack_top
    
    ; Print EAX value to debug
    push eax
    mov al, 'E'
    call serial_out
    mov al, 'A'
    call serial_out
    mov al, 'X'
    call serial_out
    mov al, ':'
    call serial_out
    pop eax
    
    ; Push multiboot info 
    push ebx    ; Multiboot info structure pointer
    push eax    ; Multiboot magic number
    
    ; Call kernel
    call kernel_main
    
    ; In case kernel returns
.hang:
    cli         ; Disable interrupts
    hlt         ; Halt the CPU
    jmp .hang   ; Jump back to .hang if we ever wake up