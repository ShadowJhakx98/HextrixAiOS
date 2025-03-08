// src/interrupts.c
#include "interrupts.h"
#include "io.h"
#include "terminal.h"
#include "scheduler.h"

// Timer ticks counter
volatile uint32_t timer_ticks = 0;

// Initialize interrupts - but actually disable them completely
void interrupts_init(void) {
    // Disable all interrupts in the PIC
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    
    // Make sure interrupts are disabled at CPU level
    asm volatile("cli");
    
    // We're not setting up any IDT entries at all, since we're using polling
    // This avoids any issues with partially configured interrupt handlers
    
    terminal_writestring("Interrupts completely disabled, using polling only\n");
}

// Polling function for timer - can be called in a loop
void timer_poll(void) {
    // Read timer status - we can check port 0x40 to poll the timer
    static uint32_t last_time = 0;
    uint32_t current_time = 0;
    
    // Read the timer counter from PIT channel 0
    outb(0x43, 0x00);  // Command: channel 0, read counter
    current_time = inb(0x40);   // Read low byte
    current_time |= inb(0x40) << 8;  // Read high byte
    
    // Timer counts down, so we can detect a change
    if (current_time != last_time) {
        last_time = current_time;
        timer_ticks++;
        
        // Call scheduler tick function to handle multitasking
        scheduler_timer_tick();
    }
}

// Polling function for keyboard
int keyboard_poll(void) {
    if (inb(0x64) & 1) {  // Check if keyboard has data
        return inb(0x60);  // Return scancode
    }
    return -1;  // No key available
}

// Register an interrupt handler (stub for compatibility)
void interrupt_register_handler(uint8_t num, void (*handler)(void)) {
    // Not implemented in polling mode
    // This function is kept as a stub for API compatibility
}