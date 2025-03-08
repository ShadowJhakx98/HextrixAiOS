; src/context_switch.asm
[BITS 32]

global context_switch
extern process_get_current

section .text

; void context_switch(process_t* next_process);
; Performs a context switch from the current process to the next process
context_switch:
    ; Get function argument (next_process)
    mov edx, [esp + 4]
    
    ; Save current process context
    pushfd                      ; Save EFLAGS
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp
    
    ; Get current process structure
    call process_get_current
    cmp eax, 0                  ; Check if there's a current process
    je .load_next               ; If not, just load the next process
    
    ; Save ESP to current process's context.esp
    mov ebx, eax                ; EBX = current process
    mov ecx, esp
    mov [ebx + 24], ecx         ; Offset 24 is where context.esp is (adjust based on structure)
    
.load_next:
    ; Load next process context
    mov ebp, edx                ; EBP = next process
    
    ; Switch stacks
    mov esp, [ebp + 24]         ; ESP = next process's saved ESP (adjust offset based on structure)
    
    ; Restore registers from stack
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    popfd                       ; Restore EFLAGS
    
    ; Return to the new process's code
    ret