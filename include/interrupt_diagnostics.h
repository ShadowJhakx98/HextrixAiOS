// include/interrupt_diagnostics.h
#ifndef INTERRUPT_DIAGNOSTICS_H
#define INTERRUPT_DIAGNOSTICS_H

#include <stdint.h>

// CPU state structure for diagnostics
typedef struct {
    // General purpose registers
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    
    // Segment registers
    uint16_t cs, ds, es, fs, gs, ss;
    
    // Control registers
    uint32_t cr0, cr2, cr3, cr4;
    
    // Descriptor tables
    uint32_t gdt_base, idt_base;
    uint16_t gdt_limit, idt_limit;
    
    // PIC state
    uint8_t pic1_mask, pic2_mask;
    uint8_t pic1_irr, pic2_irr;  // Interrupt Request Register
    uint8_t pic1_isr, pic2_isr;  // In-Service Register
} cpu_state_t;

// Initialize the interrupt diagnostics module
void interrupt_diag_init(void);

// Capture the current CPU state
void interrupt_diag_capture_state(cpu_state_t* state);

// Print the current CPU state
void interrupt_diag_print_state(const cpu_state_t* state);

// Print information about the IDT
void interrupt_diag_print_idt(void);

// Print information about the GDT
void interrupt_diag_print_gdt(void);

// Print PIC state
void interrupt_diag_print_pic_state(void);

// Test interrupts with diagnostics (safe mode)
void interrupt_diag_test_interrupts(void);

// Enable a single interrupt for testing
void interrupt_diag_enable_single_interrupt(uint8_t irq);

// Install test handlers for essential interrupts
void interrupt_diag_install_test_handlers(void);

// Get the diagnostics log buffer
const char* interrupt_diag_get_log(void);

#endif