; interrupt_stubs.asm
; Assembly stubs for interrupt handlers

global isr_timer_stub
global isr_keyboard_stub
global idt_load

extern timer_handler
extern keyboard_handler

; Common interrupt handler wrapper
%macro ISR_WRAPPER 1
    pusha                   ; Push all registers
    call %1                 ; Call the C handler
    popa                    ; Restore all registers
    iret                    ; Return from interrupt
%endmacro

section .text

; Timer interrupt stub (IRQ0, INT 0x20)
isr_timer_stub:
    ISR_WRAPPER timer_handler

; Keyboard interrupt stub (IRQ1, INT 0x21)
isr_keyboard_stub:
    ISR_WRAPPER keyboard_handler

; Function to load the IDT
idt_load:
    mov eax, [esp+4]        ; Get the IDT pointer from parameter
    lidt [eax]              ; Load IDT
    sti                     ; Enable interrupts
    ret
