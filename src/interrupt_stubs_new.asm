; src/interrupt_stubs_new.asm
; New, more robust implementation of interrupt stubs

[BITS 32]

; Exports
global irq0_handler
global irq1_handler
global exception_handler
global load_idt
extern irq_handler_common
extern exception_handler_common
extern idt_load

section .text

; Exception handler stub (generic for all exceptions)
exception_handler:
    ; Save all registers
    pushad          ; Push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    push ds
    push es
    push fs
    push gs
    
    ; Set up data segments
    mov ax, 0x10    ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push interrupt number (simulated, would come from specific handler)
    push dword 0
    
    ; Call C handler
    call exception_handler_common
    
    ; Clean up stack
    add esp, 4
    
    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popad
    
    ; Return from interrupt
    iret

; IRQ0 handler (Timer)
irq0_handler:
    ; Save all registers
    pushad
    push ds
    push es
    push fs
    push gs
    
    ; Set up data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push interrupt number
    push dword 32    ; IRQ0 = INT 32
    
    ; Call C handler
    call irq_handler_common
    
    ; Clean up stack
    add esp, 4
    
    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popad
    
    ; Return from interrupt
    iret

; IRQ1 handler (Keyboard)
irq1_handler:
    ; Save all registers
    pushad
    push ds
    push es
    push fs
    push gs
    
    ; Set up data segments
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push interrupt number
    push dword 33    ; IRQ1 = INT 33
    
    ; Call C handler
    call irq_handler_common
    
    ; Clean up stack
    add esp, 4
    
    ; Restore registers
    pop gs
    pop fs
    pop es
    pop ds
    popad
    
    ; Return from interrupt
    iret

; Function to load the IDT
load_idt:
    push ebp
    mov ebp, esp
    mov eax, [ebp+8]    ; Get the pointer parameter
    lidt [eax]          ; Load IDT
    pop ebp
    ret