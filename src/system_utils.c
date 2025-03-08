// src/system_utils.c
#include "system_utils.h"
#include "terminal.h"
#include "io.h"
#include "stdio.h"

// Halt the system due to a critical error
void system_halt(void) {
    terminal_writestring("System halted due to fatal error\n");
    terminal_writestring("Press reset button to restart\n");
    
    // Disable all hardware
    // Disable all interrupts in the PIC again (just to be safe)
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    
    // Make sure interrupts are disabled
    asm volatile("cli");
    
    // Infinite loop with HLT
    while(1) {
        asm volatile("hlt");
    }
}

// Reboot the system
void system_reboot(void) {
    terminal_writestring("Rebooting system...\n");
    
    // Wait for a bit
    for (volatile int i = 0; i < 100000; i++) {
        // Just a delay
    }
    
    // Try to reboot using keyboard controller
    uint8_t good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
    }
    
    outb(0x64, 0xFE);
    
    // If that didn't work, just halt
    terminal_writestring("Reboot failed, halting system\n");
    system_halt();
}