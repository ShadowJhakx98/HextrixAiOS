// include/interrupt_init.h
#ifndef INTERRUPT_INIT_H
#define INTERRUPT_INIT_H

#include <stdint.h>

// Initialize interrupts properly (for future use)
void interrupts_init_proper(void);

// Initialize interrupts with diagnostic logging
void interrupts_init_with_diagnostic(void);

// Register an interrupt handler (this declaration is also in interrupts.h)
void interrupt_register_handler(uint8_t num, void (*handler)(void));

// Helper function to test a single interrupt with detailed logging
void test_single_interrupt(uint8_t int_num);

// Enable/disable interrupts
static inline void enable_interrupts(void) {
    asm volatile("sti");
}

static inline void disable_interrupts(void) {
    asm volatile("cli");
}

// Wait for interrupt
static inline void wait_for_interrupt(void) {
    asm volatile("sti\nhlt");
}

#endif