// include/interrupts.h
#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

// Structures (keep these for future use)
struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Timer ticks counter
extern volatile uint32_t timer_ticks;

// Core functions
void interrupts_init(void);

// Polling functions
void timer_poll(void);
int keyboard_poll(void);

// Register interrupt handler function
void interrupt_register_handler(uint8_t num, void (*handler)(void));

#endif