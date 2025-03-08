#include <stdint.h>
#include "interrupts.h"
#include "terminal.h"
#include "io.h"

// Interrupt Descriptor Table structure for 32-bit mode
struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t always0;
    uint8_t flags;
    uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Define the IDT entries and pointer
static struct idt_entry idt[256];
static struct idt_ptr idtp;

// Declare our assembly stubs
extern void isr_timer_stub(void);
extern void isr_keyboard_stub(void);
extern void idt_load(struct idt_ptr* ptr);

// Define keyboard and timer variables 
volatile uint32_t timer_ticks = 0;
volatile char keyboard_buffer[256];
volatile int keyboard_index = 0;

// C handlers that will be called from assembly stubs
void timer_handler(void) {
    timer_ticks++;
    outb(0x20, 0x20); // Send End of Interrupt (EOI) to PIC1
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    
    // Simple scancode to ASCII conversion (US layout)
    static const char scancode_map[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };
    
    // Only handle key press (ignore key release with 0x80 bit set)
    if (!(scancode & 0x80) && scancode < sizeof(scancode_map) && scancode_map[scancode] != 0) {
        char c = scancode_map[scancode];
        if (keyboard_index < 255) {
            keyboard_buffer[keyboard_index++] = c;
            
            // Echo character to screen (optional)
            terminal_putchar(c);
        }
    }
    
    outb(0x20, 0x20); // Send EOI to PIC1
}

// Initialize a gate in the IDT
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = (base & 0xFFFF);
    idt[num].base_hi = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

// Register an interrupt handler
void interrupt_register_handler(int num, void (*handler)(void)) {
    // This function is now simplified since we're using assembly stubs
    if (num >= 0 && num < 256) {
        // We can only register for interrupt vectors that have assembly stubs
        // For now, we'll just print a message
        terminal_writestring("Handler registration requested for interrupt: ");
        char num_str[4];
        num_str[0] = '0' + (num / 100);
        num_str[1] = '0' + ((num / 10) % 10);
        num_str[2] = '0' + (num % 10);
        num_str[3] = '\0';
        terminal_writestring(num_str);
        terminal_writestring("\n");
    }
}

// Initialize the interrupt system
void init_interrupts(void) {
    // Set up the IDT pointer
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;
    
    // Clear out the entire IDT
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    // Remap PIC interrupts to avoid conflicts
    // ICW1: begin initialization
    outb(0x20, 0x11);  // PIC1
    outb(0xA0, 0x11);  // PIC2
    
    // ICW2: define PIC vectors
    outb(0x21, 0x20);  // PIC1: starts at 32 (0x20)
    outb(0xA1, 0x28);  // PIC2: starts at 40 (0x28)
    
    // ICW3: setup cascading
    outb(0x21, 0x04);  // PIC1: has PIC2 at IRQ2
    outb(0xA1, 0x02);  // PIC2: cascade identity
    
    // ICW4: additional environment info
    outb(0x21, 0x01);  // PIC1
    outb(0xA1, 0x01);  // PIC2
    
    // Mask all interrupts initially
    outb(0x21, 0xFF);  // PIC1: mask all
    outb(0xA1, 0xFF);  // PIC2: mask all
    
    // Set up interrupt handlers
    // 0x8E = 10001110 = Present, Ring 0, 32-bit Interrupt Gate
    idt_set_gate(0x20, (uint32_t)isr_timer_stub, 0x08, 0x8E);    // Timer (IRQ0)
    idt_set_gate(0x21, (uint32_t)isr_keyboard_stub, 0x08, 0x8E); // Keyboard (IRQ1)
    
    // Load the IDT using assembly function
    idt_load(&idtp);
    
    // Now enable only the interrupts we want
    outb(0x21, 0xFC);  // Enable IRQ0 (timer) and IRQ1 (keyboard)
    
    terminal_writestring("Interrupts initialized\n");
}

// Alias for init_interrupts
void interrupts_init(void) {
    init_interrupts();
}