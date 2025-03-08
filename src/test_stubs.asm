; test_stubs.asm - Ultra-simplified for testing
global test_timer_stub
global test_load_idt

section .text

; Super minimal timer stub
test_timer_stub:
    ; Push EAX to save it temporarily
    push eax
    ; Send EOI directly from assembly
    mov al, 0x20
    out 0x20, al
    ; Restore EAX
    pop eax
    ; Return from interrupt
    iret

; Simplified IDT loader
test_load_idt:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]        ; Get pointer parameter
    lidt [eax]              ; Load IDT
    pop ebp
    ret