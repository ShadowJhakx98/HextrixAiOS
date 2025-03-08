// src/kernel.c
#include "terminal.h"
#include "kmalloc.h"
#include "interrupts.h"
#include "shell.h"
#include "fs.h"

void kernel_main(unsigned int magic, unsigned int addr) {
    // Initialize terminal
    terminal_initialize();
    
    // Print welcome message
    terminal_writestring("Initializing Hextrix OS (32-bit) - Polling Shell\n");
    
    // Check multiboot magic
    if (magic != 0x2BADB002) {
        terminal_writestring("Invalid multiboot magic number!\n");
    } else {
        terminal_writestring("Multiboot OK\n");
    }
    
    // Initialize memory management
    kmalloc_init();
    terminal_writestring("Memory management initialized\n");
    
    // Initialize "interrupts" (actually disable them)
    interrupts_init();
    
    // Initialize file system
    fs_init();
    terminal_writestring("File system initialized\n");
    
    // Initialize and run the shell
    shell_init();
    shell_run();
    
    // We should never get here
    terminal_writestring("Shell exited. System halted.\n");
    
    // Enter an infinite loop
    while (1) {
        __asm__ volatile("hlt");
    }
}