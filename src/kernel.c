// src/kernel.c
#include <stdbool.h>
#include <stdint.h>
#include "terminal.h"
#include "kmalloc.h"
#include "interrupts.h"
#include "shell.h"
#include "fs.h"
#include "process.h"
#include "scheduler.h"
#include "memory.h"
#include "stdio.h"
#include "system_utils.h"
#include "hal.h"
#include "gui/desktop.h"

// Global stack canary variable
uint32_t __stack_canary = 0xDEADBEEF;

// Function declarations with correct return types
void serial_print(const char *str);
void fallback_shell_loop(void);
static void print_hex(unsigned int num);

// Debug macro for serial output
#define SERIAL_DEBUG(msg) serial_print(msg)

// Serial debugging initialization
void serial_init() {
    // Initialize COM1 serial port (0x3F8)
    outb(0x3F8 + 1, 0x00);    // Disable interrupts
    outb(0x3F8 + 3, 0x80);    // Enable DLAB (Divisor Latch Access Bit)
    outb(0x3F8 + 0, 0x03);    // Set divisor to 3 (38400 baud)
    outb(0x3F8 + 1, 0x00);    // High byte of divisor
    outb(0x3F8 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7);    // Enable FIFO, clear them, 14-byte threshold
}

// Serial print function
void serial_print(const char* str) {
    while (*str) {
        // Wait for the transmit buffer to be empty
        while (!(inb(0x3F8 + 5) & 0x20));
        outb(0x3F8, *str++);
    }
}

// Helper function to print hex values
static void print_hex(unsigned int num) {
    static const char hex_chars[] = "0123456789ABCDEF";
    char output[11]; // 0x + 8 digits + null terminator
    
    output[0] = '0';
    output[1] = 'x';
    
    for (int i = 0; i < 8; i++) {
        output[2 + i] = hex_chars[(num >> (28 - i * 4)) & 0xF];
    }
    
    output[10] = '\0';
    serial_print(output);
}

// In kernel.c
bool safe_desktop_init() {
    SERIAL_DEBUG("safe_desktop_init: Attempting GUI initialization...\n"); // DEBUG - ENTRY

    // Check framebuffer readiness
    if (!hal_framebuffer_is_ready()) {
        SERIAL_DEBUG("safe_desktop_init: Framebuffer not ready. GUI initialization failed.\n");
        return false;
    }
    SERIAL_DEBUG("safe_desktop_init: Framebuffer is ready for GUI initialization.\n");

    // Initialize desktop components
    SERIAL_DEBUG("safe_desktop_init: Calling desktop_init()...\n");
    int init_result = desktop_init(); // <-- CRUCIAL: CALL TO desktop_init()
    SERIAL_DEBUG("safe_desktop_init: desktop_init() returned: ");
    print_hex(init_result);
    SERIAL_DEBUG("\n");

    if (init_result != 0) {
        SERIAL_DEBUG("safe_desktop_init: Desktop initialization failed with error code.\n");
        return false;
    }

    // Start desktop environment
    SERIAL_DEBUG("safe_desktop_init: Calling desktop_run()...\n");
    desktop_run(); // <-- CRUCIAL: CALL TO desktop_run() AFTER desktop_init() succeeds
    SERIAL_DEBUG("safe_desktop_init: desktop_run() returned unexpectedly!\n");
    return false;
}

// Fallback shell loop with safety checks
// Fallback shell loop with safety checks
void fallback_shell_loop() {
    SERIAL_DEBUG("Entering fallback shell loop...\n");
    
    while (1) {
        // Stack canary check
        if (__stack_canary != 0xDEADBEEF) {
            // Check stack canary value
            SERIAL_DEBUG("CRITICAL: Stack overflow detected!\n");
            SERIAL_DEBUG("Stack canary value corrupted! Expected 0xDEADBEEF, but found: ");
            print_hex(__stack_canary);
            SERIAL_DEBUG("\n");
            system_halt(); // Halt system to prevent further damage
        }
        
        // Perform safety checks
        if (!hal_is_system_stable()) {
            SERIAL_DEBUG("System instability detected. Halting.\n");
            system_halt();
        }
        
        // Poll timer to keep system responsive
        hal_timer_poll();
        
        // Keyboard handling with error checking
        if (hal_keyboard_is_key_available()) {
            int scancode = hal_keyboard_read();
            if (scancode == -1) {
                SERIAL_DEBUG("Keyboard read error.\n");
                continue;
            }
            
            // Handle key in shell
            shell_handle_key(scancode);
        }
        
        // Yield to prevent tight looping
        scheduler_yield();
    }
}
extern uint32_t __stack_canary; // Declare stack canary symbol

// Enhanced kernel main with robust error handling
// Enhanced kernel main with robust error handling
void kernel_main(unsigned int magic, unsigned int addr) {
    // Early serial debugging initialization
    serial_init();
    SERIAL_DEBUG("Hextrix OS Booting...\n");
    
    // Initialize terminal
    terminal_initialize();
    SERIAL_DEBUG("Terminal initialized.\n");
    
    // Stack canary initialization
    __stack_canary = 0xDEADBEEF; // Initialize stack canary value
    SERIAL_DEBUG("Stack canary initialized to: ");
    print_hex(__stack_canary);
    SERIAL_DEBUG("\n");
    
    // Multiboot validation with detailed logging
    if (magic != 0x2BADB000) {
        SERIAL_DEBUG("CRITICAL: Invalid multiboot magic number!\n");
        terminal_writestring("Invalid multiboot magic number!\n");
        system_halt();
    }
    SERIAL_DEBUG("Multiboot validation passed.\n");
    
    // Memory management initialization with error checking
    if (kmalloc_init() != 0) {
        SERIAL_DEBUG("CRITICAL: Memory management initialization failed!\n");
        terminal_writestring("Memory initialization failed!\n");
        system_halt();
    }
    SERIAL_DEBUG("Memory management initialized.\n");
    
    // Paging initialization with error handling
    if (init_paging() != 0) {
        SERIAL_DEBUG("CRITICAL: Paging initialization failed!\n");
        terminal_writestring("Paging initialization failed!\n");
        system_halt();
    }
    SERIAL_DEBUG("Paging initialized.\n");
    
    // HAL initialization with comprehensive checks
    SERIAL_DEBUG("Starting HAL initialization...\n");
    int hal_result = hal_init();
    if (hal_result != 0) {
        SERIAL_DEBUG("CRITICAL: HAL core initialization failed with code: ");
        print_hex(hal_result);
        SERIAL_DEBUG("\n");
        terminal_writestring("HAL initialization failed!\n");
        system_halt();
    }
    SERIAL_DEBUG("HAL core initialized successfully.\n");
    
    SERIAL_DEBUG("Starting HAL device initialization...\n");
    int hal_devices_result = hal_init_devices();
    if (hal_devices_result != 0) {
        SERIAL_DEBUG("CRITICAL: HAL device initialization failed with code: ");
        print_hex(hal_devices_result);
        SERIAL_DEBUG("\n");
        terminal_writestring("HAL device initialization failed!\n");
        system_halt();
    }
    SERIAL_DEBUG("Hardware Abstraction Layer fully initialized.\n");
    
    // Timer and scheduler setup
    SERIAL_DEBUG("Registering scheduler timer callback...\n");
    hal_timer_register_callback(scheduler_timer_tick);
    SERIAL_DEBUG("Timer callback registered.\n");
    
    // File system initialization
    SERIAL_DEBUG("Starting filesystem initialization...\n");
    int fs_result = fs_init();
    SERIAL_DEBUG("Filesystem initialization result: ");
    print_hex(fs_result);
    SERIAL_DEBUG("\n");
    
    if (fs_result != 0) {
        SERIAL_DEBUG("WARNING: File system initialization failed.\n");
        // Continue boot, but log the issue
    } else {
        SERIAL_DEBUG("Filesystem initialized successfully.\n");
    }
    
    // Process and scheduler initialization
    SERIAL_DEBUG("Starting process initialization...\n");
    process_init();
    SERIAL_DEBUG("Process initialization complete.\n");
    
    SERIAL_DEBUG("Starting scheduler initialization...\n");
    scheduler_init();
    SERIAL_DEBUG("Scheduler initialization complete.\n");
    SERIAL_DEBUG("Process scheduler initialized.\n");
    
    // Attempt GUI initialization
    SERIAL_DEBUG("About to attempt GUI desktop initialization...\n");
    bool gui_success = safe_desktop_init();
    SERIAL_DEBUG("GUI initialization attempt completed with result: ");
    SERIAL_DEBUG(gui_success ? "SUCCESS" : "FAILURE");
    SERIAL_DEBUG("\n");
    
    if (!gui_success) {
        SERIAL_DEBUG("GUI initialization failed. Falling back to shell.\n");
        terminal_writestring("GUI failed to start. Entering fallback shell.\n");
        
        // Fallback to shell with safety mechanisms
        SERIAL_DEBUG("Initializing fallback shell...\n");
        shell_init();
        SERIAL_DEBUG("Shell initialized, entering shell loop...\n");
        fallback_shell_loop();
    } else {
        SERIAL_DEBUG("GUI initialization succeeded, system should now be in GUI mode.\n");
        wm_update();  // Call wm_update() here, but note that this might not be sufficient for a fully functional GUI
        // If we reach here with successful GUI but nothing happens, we might have a loop issue
        SERIAL_DEBUG("WARNING: Control returned from GUI but GUI was successful. This indicates a potential issue.\n");
    }
    
    // Final safety net (should never reach here)
    SERIAL_DEBUG("Unexpected kernel exit. Entering fallback shell.\n");
    terminal_writestring("Unexpected system state. Entering fallback shell.\n");
    shell_init();
    fallback_shell_loop();
}