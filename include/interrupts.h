#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#include <stdint.h>

// Initialize the interrupt system
void init_interrupts(void);
void interrupts_init(void);

// Register a handler for an interrupt
void interrupt_register_handler(int num, void (*handler)(void));

// Direct access to IDT (used by keyboard handler)
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

// Shared variables
extern volatile uint32_t timer_ticks;
extern volatile char keyboard_buffer[256];
extern volatile int keyboard_index;

#endif