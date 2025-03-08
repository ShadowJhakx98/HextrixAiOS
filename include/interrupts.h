#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#include <stdint.h>

void init_interrupts(void);
extern volatile uint32_t timer_ticks;
extern volatile char keyboard_buffer[256];
extern volatile int keyboard_index;

// include/interrupts.h
// Add this function declaration
void interrupt_register_handler(int num, void (*handler)(void));
#endif