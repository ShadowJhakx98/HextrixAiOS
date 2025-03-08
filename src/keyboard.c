// src/keyboard.c - Updated version
#include "keyboard.h"
#include "interrupts.h"
#include "terminal.h"
#include "io.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_BUFFER_SIZE 256

// Use the existing buffer defined in interrupts.h
// We'll maintain our own buffer indices
static int buffer_head = 0;
static int buffer_tail = 0;

// Scancode to ASCII mapping (US layout)
static const char scancode_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// Handle keyboard interrupt
static void keyboard_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Only handle key press events (ignore key release)
    if (!(scancode & 0x80)) {
        char c = scancode_ascii[scancode];
        if (c) {
            // Add character to buffer if it's a valid ASCII character
            int next_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
            if (next_head != buffer_tail) {  // Ensure buffer isn't full
                keyboard_buffer[buffer_head] = c;
                buffer_head = next_head;
            }
            
            // Echo character to screen (optional)
            terminal_putchar(c);
        }
    }
    
    // Send End of Interrupt
    outb(0x20, 0x20);
}

void keyboard_init(void) {
    // Register interrupt handler for keyboard (IRQ 1)
    interrupt_register_handler(33, keyboard_handler);
    
    // Enable keyboard interrupt
    outb(0x21, inb(0x21) & ~2);
}

int keyboard_getchar(void) {
    if (buffer_head == buffer_tail) {
        return -1;  // No character available
    }
    
    char c = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

char keyboard_read(void) {
    while (buffer_head == buffer_tail) {
        // Wait for a key to be pressed
        __asm__ volatile("hlt");
    }
    
    return keyboard_getchar();
}

int keyboard_is_key_available(void) {
    return buffer_head != buffer_tail;
}