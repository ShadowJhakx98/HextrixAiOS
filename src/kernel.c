// src/kernel.c
#include "terminal.h"
#include "kmalloc.h"
#include "interrupts.h"
#include "shell.h"
#include "fs.h"
#include "process.h"
#include "scheduler.h"
#include "memory.h"
#include "stdio.h"  // Added for terminal_printf
#include "system_utils.h" // Added for system_halt
#include "hal.h" // Added for HAL functions
#include "gui/desktop.h" // Add this at the top of the file

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

// Memory test function
void test_memory_protection(void) {
    terminal_writestring("Testing memory protection...\n");
    
    // Test valid memory access
    uint32_t* valid_ptr = (uint32_t*)(0x100000); // 1MB (heap start)
    if (is_valid_access((uint32_t)valid_ptr, MEM_PROT_READ | MEM_PROT_WRITE)) {
        terminal_writestring("Valid heap access check passed\n");
        *valid_ptr = 0x12345678; // This should work
        terminal_printf("Value at 0x%x: 0x%x\n", valid_ptr, *valid_ptr);
    } else {
        terminal_writestring("ERROR: Valid heap access check failed\n");
    }
    
    // Test invalid memory access (would cause a page fault if we enable the handler)
    uint32_t* invalid_ptr = (uint32_t*)(0x10000000); // 256MB (unmapped)
    if (!is_valid_access((uint32_t)invalid_ptr, MEM_PROT_READ)) {
        terminal_writestring("Invalid memory access check passed\n");
    } else {
        terminal_writestring("ERROR: Invalid memory access check failed\n");
    }
    
    // Test memory protection on valid address
    uint32_t* kernel_ptr = (uint32_t*)(0x500); // Kernel space
    if (!is_valid_access((uint32_t)kernel_ptr, MEM_PROT_WRITE | MEM_PROT_USER)) {
        terminal_writestring("Kernel memory protection check passed (user cannot write to kernel space)\n");
    } else {
        terminal_writestring("ERROR: Kernel memory protection check failed (user can write to kernel space)\n");
    }
    
    terminal_writestring("Memory protection test complete\n");
}

// Need to declare this function if it's not exposed in scheduler.h
extern void scheduler_timer_tick(void);

void kernel_main(unsigned int magic, unsigned int addr) {
    // Initialize terminal
    terminal_initialize();
    
    // Print welcome message
    terminal_writestring("Initializing Hextrix OS (32-bit) v0.4.0-beta - GUI Edition\n");
    
    // Check multiboot magic
    if (magic != 0x2BADB002) {
        terminal_writestring("Invalid multiboot magic number!\n");
    } else {
        terminal_writestring("Multiboot OK\n");
    }
    
    // Initialize memory management
    kmalloc_init();
    terminal_writestring("Memory management initialized\n");
    
    // Initialize paging with memory protection
    init_paging();
    terminal_writestring("Paging initialized\n");
    
    // Memory protection is disabled by default for stability
    terminal_writestring("Memory protection is available but disabled by default\n");
    terminal_writestring("Use 'memenable' command to enable it when ready\n");
    
    // Initialize HAL
    hal_init();
    hal_init_devices();
    terminal_writestring("Hardware Abstraction Layer initialized\n");
    
    // Register timer callback for scheduler
    hal_timer_register_callback(scheduler_timer_tick);
    
    // Initialize file system
    fs_init();
    terminal_writestring("File system initialized\n");
    
    // Initialize process management
    process_init();
    scheduler_init();
    terminal_writestring("Process scheduler initialized\n");
    
    // Launch the desktop GUI
    terminal_writestring("Starting GUI desktop environment...\n");
    desktop_run();
    
    // If desktop_run() ever returns, fall back to the shell
    terminal_writestring("GUI desktop exited. Falling back to shell.\n");
    shell_init();
    
    // Main loop (fallback if GUI fails)
    while (1) {
        // Poll the timer
        hal_timer_poll();
        
        // Poll keyboard and handle input
        hal_keyboard_poll();
        
        if (hal_keyboard_is_key_available()) {
            int scancode = hal_keyboard_read();
            shell_handle_key(scancode);
        }
        
        // Short delay to reduce CPU usage in emulator
        for (volatile int i = 0; i < 10000; i++) {}
    }
}