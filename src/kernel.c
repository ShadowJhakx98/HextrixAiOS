void kernel_main(unsigned int magic, unsigned int addr);
#include "terminal.h"
#include "memory.h"
#include "scheduler.h"
#include "kmalloc.h"
#include "interrupts.h"
#include "keyboard.h"
#include "fs.h"
#include "shell.h"

void kernel_main(unsigned int magic, unsigned int addr) {
    // Initialize terminal
    terminal_initialize();
    
    // Print welcome message
    terminal_writestring("Initializing Hextrix OS (32-bit)\n");
    
    // Check multiboot magic
    if (magic != 0x2BADB002) {
        terminal_writestring("Invalid multiboot magic number!\n");
    } else {
        terminal_writestring("Multiboot OK\n");
    }
    
    // Initialize memory management
    kmalloc_init();
    terminal_writestring("Memory management initialized\n");
    
    // Initialize interrupt system
    interrupts_init();
    terminal_writestring("Interrupts initialized\n");
    
    // Initialize keyboard
    keyboard_init();
    terminal_writestring("Keyboard initialized\n");
    
    // Initialize file system
    fs_init();
    terminal_writestring("File system initialized\n");
    
    // Initialize shell
    shell_init();
    
    // Run the shell (this doesn't return)
    shell_run();
    
    // We should never get here
    while (1) {
        // Halt CPU
        __asm__ volatile("hlt");
    }
}