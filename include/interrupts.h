// include/interrupts.h
#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

// Timer ticks counter
extern volatile uint32_t timer_ticks;

// Core functions
void interrupts_init(void);

// Polling functions
void timer_poll(void);
int keyboard_poll(void);

// Register interrupt handler function (stub for compatibility)
void interrupt_register_handler(uint8_t num, void (*handler)(void));

#endif