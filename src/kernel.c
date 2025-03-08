// src/kernel.c
#include "terminal.h"
#include "kmalloc.h"
#include "interrupts.h"
#include "shell.h"
#include "fs.h"
#include "process.h"
#include "scheduler.h"

// Demo task 1
static void demo_task1(void) {
    while (1) {
        terminal_writestring("Task 1 running\n");
        
        // Simulate some work
        for (volatile int i = 0; i < 10000000; i++) {}
        
        // Yield to other processes
        scheduler_yield();
    }
}

// Demo task 2
static void demo_task2(void) {
    while (1) {
        terminal_writestring("Task 2 running\n");
        
        // Simulate some work
        for (volatile int i = 0; i < 15000000; i++) {}
        
        // Yield to other processes
        scheduler_yield();
    }
}

// Shell task
static void shell_task(void) {
    shell_init();
    shell_run();
    
    // Should never return, but if it does:
    terminal_writestring("Shell exited. System halted.\n");
    while (1) {
        scheduler_yield();
    }
}

void kernel_main(unsigned int magic, unsigned int addr) {
    // Initialize terminal
    terminal_initialize();
    
    // Print welcome message
    terminal_writestring("Initializing Hextrix OS (32-bit) v0.3.3 - Enhanced Scheduler\n");
    
    // Check multiboot magic
    if (magic != 0x2BADB002) {
        terminal_writestring("Invalid multiboot magic number!\n");
    } else {
        terminal_writestring("Multiboot OK\n");
    }
    
    // Initialize memory management
    kmalloc_init();
    terminal_writestring("Memory management initialized\n");
    
    // Initialize interrupts (actually disable them since we're using polling)
    interrupts_init();
    
    // Initialize file system
    fs_init();
    terminal_writestring("File system initialized\n");
    
    // Initialize process management
    process_init();
    scheduler_init();
    terminal_writestring("Process scheduler initialized\n");
    
    // Instead of creating multiple processes, let's start simpler
    // Just start the shell directly for now to debug the issue
    shell_init();
    terminal_writestring("Shell initialized\n");
    
    // Main loop
    while (1) {
        // Poll the timer
        timer_poll();
        
        // Handle shell input directly
        int scancode = keyboard_poll();
        if (scancode > 0) {
            shell_handle_key(scancode);
        }
        
        // Short delay to reduce CPU usage in emulator
        for (volatile int i = 0; i < 10000; i++) {}
    }
}