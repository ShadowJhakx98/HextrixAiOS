; Multiboot header for GRUB
MULTIBOOT_PAGE_ALIGN    equ 1<<0
MULTIBOOT_MEMORY_INFO   equ 1<<1
MULTIBOOT_HEADER_MAGIC  equ 0x1BADB002
MULTIBOOT_HEADER_FLAGS  equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM      equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

section .multiboot
align 4
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd MULTIBOOT_CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384 ; 16 KB
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; Setup stack
    mov esp, stack_top
    
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
