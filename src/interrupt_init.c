// src/interrupt_init.c
// This file contains a proper implementation of interrupt handling
// It should be integrated only after testing with interrupt_diagnostics

#include "interrupts.h"
#include "io.h"
#include "terminal.h"
#include "stdio.h"
#include "interrupt_diagnostics.h"

// IDT entry structure
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

// IDT pointer structure
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

// Define our IDT
#define IDT_SIZE 256
static struct idt_entry idt[IDT_SIZE] __attribute__((aligned(8)));
static struct idt_ptr idtp;

// Timer ticks counter
volatile uint32_t timer_ticks = 0;

// Handler function pointers
static void (*interrupt_handlers[IDT_SIZE])(void) = {0};

// Default handler for unhandled interrupts
static void default_handler(void) {
    terminal_writestring("Unhandled interrupt!\n");
    
    // Send EOI to PIC for IRQs
    outb(0x20, 0x20); // Master PIC
    outb(0xA0, 0x20); // Slave PIC
}

// Helper function to set an IDT gate
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = base & 0xFFFF;
    idt[num].offset_high = (base >> 16) & 0xFFFF;
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

// ASCII representation of exception names
static const char* exception_names[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

// Exception handlers
static void exception_handler_common(void) {
    uint32_t int_no;

    // Get interrupt number from stack
    asm volatile("mov 8(%%ebp), %0" : "=r"(int_no));

    if (int_no < 32) {
        terminal_writestring("EXCEPTION: ");
        terminal_writestring(exception_names[int_no]);
        terminal_writestring("\n");
        
        // Capture diagnostic information
        cpu_state_t state;
        interrupt_diag_capture_state(&state);
        interrupt_diag_print_state(&state);
        
        // Halt the system
        terminal_writestring("System halted\n");
        for (;;) {
            asm volatile("hlt");
        }
    }
}

// IRQ common handler
static void irq_handler_common(void) {
    uint32_t int_no;

    // Get interrupt number from stack
    asm volatile("mov 8(%%ebp), %0" : "=r"(int_no));

    // Calculate IRQ number
    uint32_t irq = int_no - 32;

    // Check if we have a handler for this IRQ
    if (interrupt_handlers[int_no] != 0) {
        // Call the handler
        interrupt_handlers[int_no]();
    } else {
        // No handler registered
        terminal_printf("Unhandled IRQ%d\n", irq);
    }

    // Send EOI to PIC
    if (irq >= 8) {
        // Send EOI to slave PIC
        outb(0xA0, 0x20);
    }
    
    // Send EOI to master PIC
    outb(0x20, 0x20);
}

// Timer (IRQ0) handler
static void timer_handler(void) {
    timer_ticks++;
}

// Keyboard (IRQ1) handler
static void keyboard_handler(void) {
    // Read the scancode from the keyboard
    uint8_t scancode = inb(0x60);
    
    // Process the scancode here
    // For diagnostics, just print the scancode
    terminal_printf("Keyboard: Scancode 0x%x\n", scancode);
}

// IRQ handlers need to be defined in assembly
extern void irq0_handler(void);
extern void irq1_handler(void);
extern void exception_handler(void);

// Initialize the IDT with proper handlers
static void init_idt(void) {
    // Set up IDT pointer
    idtp.limit = (sizeof(struct idt_entry) * IDT_SIZE) - 1;
    idtp.base = (uint32_t)&idt;
    
    // Clear out the entire IDT
    for (int i = 0; i < IDT_SIZE; i++) {
        idt_set_gate(i, (uint32_t)exception_handler, 0x08, 0x8E);
    }
    
    // Setup IRQ handlers (we'll map IRQs 0-15 to interrupts 32-47)
    idt_set_gate(32, (uint32_t)irq0_handler, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1_handler, 0x08, 0x8E);
    
    // Load IDT
    asm volatile("lidt %0" : : "m"(idtp));
}

// Initialize the Programmable Interrupt Controller (PIC)
static void init_pic(void) {
    // Initialize both PICs
    
    // ICW1: Initialize + ICW4 needed
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    
    // ICW2: Remap IRQs
    outb(0x21, 0x20); // Master PIC: IRQ0-7 -> INT 32-39
    outb(0xA1, 0x28); // Slave PIC: IRQ8-15 -> INT 40-47
    
    // ICW3: Tell PICs about each other
    outb(0x21, 0x04); // Master PIC: IRQ2 has a slave
    outb(0xA1, 0x02); // Slave PIC: Cascade identity (I'm slave on IRQ2)
    
    // ICW4: Additional configuration
    outb(0x21, 0x01); // 8086 mode
    outb(0xA1, 0x01); // 8086 mode
    
    // Mask all interrupts except IRQ2 (cascade)
    outb(0x21, 0xFB); // Enable only IRQ2 (0xFB = 1111 1011)
    outb(0xA1, 0xFF); // Mask all slave interrupts
}

// Register an interrupt handler
void interrupt_register_handler(uint8_t num, void (*handler)(void)) {
    interrupt_handlers[num] = handler;
    
    // If this is an IRQ, update the PIC mask
    if (num >= 32 && num < 48) {
        uint8_t irq = num - 32;
        
        if (irq < 8) {
            // IRQ on master PIC
            uint8_t mask = inb(0x21);
            mask &= ~(1 << irq); // Clear bit to enable IRQ
            outb(0x21, mask);
        } else {
            // IRQ on slave PIC
            
            // First ensure IRQ2 is enabled on master (cascade)
            uint8_t master_mask = inb(0x21);
            master_mask &= ~(1 << 2); // Clear bit 2 to enable IRQ2 (cascade)
            outb(0x21, master_mask);
            
            // Now enable the specific IRQ on slave
            uint8_t slave_mask = inb(0xA1);
            slave_mask &= ~(1 << (irq - 8)); // Clear bit to enable IRQ
            outb(0xA1, slave_mask);
        }
    }
}

// Initialize interrupts
void interrupts_init_proper(void) {
    // Initialize the IDT
    init_idt();
    
    // Initialize the PIC
    init_pic();
    
    // Register handlers
    interrupt_register_handler(32, timer_handler);
    interrupt_register_handler(33, keyboard_handler);
    
    // Enable interrupts
    asm volatile("sti");
    
    terminal_writestring("Interrupts initialized successfully\n");
}

// Function to simulate how interrupt_init.c would be integrated into interrupts.c
void interrupts_init_with_diagnostic(void) {
    // Initialize the diagnostic module
    interrupt_diag_init();
    
    // Capture initial CPU state
    cpu_state_t initial_state;
    interrupt_diag_capture_state(&initial_state);
    interrupt_diag_print_state(&initial_state);
    
    // Print IDT and GDT information
    interrupt_diag_print_idt();
    interrupt_diag_print_gdt();
    
    // Initialize the PIC and IDT
    init_pic();
    init_idt();
    
    // Register handlers
    interrupt_register_handler(32, timer_handler);
    interrupt_register_handler(33, keyboard_handler);
    
    // Capture post-setup CPU state
    cpu_state_t post_setup_state;
    interrupt_diag_capture_state(&post_setup_state);
    interrupt_diag_print_state(&post_setup_state);
    
    // Enable interrupts
    asm volatile("sti");
    
    terminal_writestring("Interrupts initialized with diagnostics\n");
}