#include "keyboard.h"
#include "interrupts.h"
#include "terminal.h"
#include "io.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_BUFFER_SIZE 256

// Local buffer for keyboard input  
static char local_keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int local_buffer_head = 0;
static int local_buffer_tail = 0;

// Scancode to ASCII mapping (US layout)
static const char scancode_ascii[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// This is a dedicated keyboard handler - it doesn't depend on external handlers
static void keyboard_interrupt_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Only handle key press events (ignore key release with bit 7 set)
    if (!(scancode & 0x80) && scancode < sizeof(scancode_ascii) && scancode_ascii[scancode]) {
        char c = scancode_ascii[scancode];
        
        // Add character to local buffer
        int next_head = (local_buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
        if (next_head != local_buffer_tail) {  // Ensure buffer isn't full
            local_keyboard_buffer[local_buffer_head] = c;
            local_buffer_head = next_head;
        }
        
        // Echo character to screen 
        terminal_putchar(c);
    }
    
    // Send End of Interrupt
    outb(0x20, 0x20);
}

void keyboard_init(void) {
    // Initialize buffer
    local_buffer_head = local_buffer_tail = 0;
    
    // Register our handler for IRQ1 (keyboard) - interrupt 33 (0x21)
    interrupt_register_handler(0x21, keyboard_interrupt_handler);
    
    // Enable keyboard IRQ
    outb(0x21, inb(0x21) & ~2);  // Clear bit 1 to enable IRQ1
}

int keyboard_getchar(void) {
    if (local_buffer_head == local_buffer_tail) {
        return -1;  // No character available
    }
    
    char c = local_keyboard_buffer[local_buffer_tail];
    local_buffer_tail = (local_buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

char keyboard_read(void) {
    while (local_buffer_head == local_buffer_tail) {
        // Wait for a key to be pressed
        asm volatile("hlt");
    }
    
    char c = local_keyboard_buffer[local_buffer_tail];
    local_buffer_tail = (local_buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

int keyboard_is_key_available(void) {
    return local_buffer_head != local_buffer_tail;
}