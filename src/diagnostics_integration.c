// src/diagnostics_integration.c
// Integration between the main OS and diagnostics tools

#include "terminal.h"
#include "stdio.h"
#include "interrupt_diagnostics.h"
#include "shell.h"
#include "system_utils.h"

// Function to be called from shell to run diagnostics
void run_interrupt_diagnostics(void) {
    terminal_writestring("Starting interrupt diagnostics...\n");
    
    // Initialize the diagnostics module
    interrupt_diag_init();
    
    // Capture initial CPU state
    cpu_state_t state;
    interrupt_diag_capture_state(&state);
    
    // Print CPU state
    terminal_writestring("Current CPU state:\n");
    interrupt_diag_print_state(&state);
    
    // Print IDT information
    terminal_writestring("IDT status:\n");
    interrupt_diag_print_idt();
    
    // Print GDT information
    terminal_writestring("GDT status:\n");
    interrupt_diag_print_gdt();
    
    // Print PIC state
    terminal_writestring("PIC state:\n");
    interrupt_diag_print_pic_state();
    
    // Run safe diagnostic test
    terminal_writestring("Running safe diagnostic test (without enabling real interrupts)...\n");
    interrupt_diag_test_interrupts();
    
    terminal_writestring("Interrupt diagnostics completed. Check log for details.\n");
}