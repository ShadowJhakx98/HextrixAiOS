; interrupt_stubs.asm - Simplified version
global isr_timer_stub
global isr_keyboard_stub
global idt_load

extern timer_handler
extern keyboard_handler

section .text

; Timer interrupt stub (IRQ0)
isr_timer_stub:
    pushad                  ; Save all registers
    cld                     ; Clear direction flag
    call timer_handler      ; Call C handler
    popad                   ; Restore all registers
    iretd                   ; Return from interrupt

; Keyboard interrupt stub (IRQ1)
isr_keyboard_stub:
    pushad                  ; Save all registers
    cld                     ; Clear direction flag
    call keyboard_handler   ; Call C handler
    popad                   ; Restore all registers
    iretd                   ; Return from interrupt

; Function to load the IDT
idt_load:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]        ; Get the pointer parameter
    lidt [eax]              ; Load IDT
    pop ebp
    ret