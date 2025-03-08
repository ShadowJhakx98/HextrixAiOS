; test_stubs.asm - Ultra-minimal version with no C function calls
global test_timer_stub
global test_load_idt
global timer_count       ; Export timer_count as a global variable

section .data
timer_count: db 0        ; Define a global counter for timer interrupts

section .text

; Ultra-minimal timer stub that doesn't call any C code
test_timer_stub:
    pusha                ; Save all registers
    
    ; Simply increment the counter - no C function call
    inc byte [timer_count]
    
    ; Send EOI directly
    mov al, 0x20
    out 0x20, al
    
    popa                 ; Restore all registers
    iret                 ; Return from interrupt

; Simplified IDT loader
test_load_idt:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]     ; Get pointer parameter
    lidt [eax]           ; Load IDT
    pop ebp
    ret