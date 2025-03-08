// src/interrupt_diagnostics.c
#include "interrupt_diagnostics.h"
#include "terminal.h"
#include "stdio.h"
#include "io.h"
#include "string.h"
#include <stdarg.h>  // Added for va_list

// Log buffer for diagnostics
#define DIAG_LOG_SIZE 4096
static char diag_log_buffer[DIAG_LOG_SIZE];
static int diag_log_pos = 0;

// Add to log buffer
static void diag_log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    // Calculate remaining space in buffer
    int remaining = DIAG_LOG_SIZE - diag_log_pos - 1;
    
    if (remaining > 0) {
        char temp_buffer[256];
        int len = 0;
        
        // Format message to temporary buffer
        for (int i = 0; format[i] != '\0' && len < 255; i++) {
            if (format[i] == '%') {
                i++;
                switch (format[i]) {
                    case 'd': {
                        int val = va_arg(args, int);
                        // Simple int to string
                        if (val == 0) {
                            temp_buffer[len++] = '0';
                        } else {
                            int is_negative = 0;
                            if (val < 0) {
                                is_negative = 1;
                                val = -val;
                            }
                            
                            // Convert in reverse
                            char rev_digits[12];
                            int rev_len = 0;
                            while (val > 0 && rev_len < 11) {
                                rev_digits[rev_len++] = '0' + (val % 10);
                                val /= 10;
                            }
                            
                            // Add negative sign if needed
                            if (is_negative) {
                                temp_buffer[len++] = '-';
                            }
                            
                            // Add digits in correct order
                            while (rev_len > 0) {
                                temp_buffer[len++] = rev_digits[--rev_len];
                            }
                        }
                        break;
                    }
                    case 'x': {
                        // Hex format
                        uint32_t val = va_arg(args, uint32_t);
                        
                        // Add 0x prefix
                        temp_buffer[len++] = '0';
                        temp_buffer[len++] = 'x';
                        
                        // Print at least one digit
                        int printed = 0;
                        for (int j = 7; j >= 0; j--) {
                            int digit = (val >> (j * 4)) & 0xF;
                            
                            if (digit != 0 || printed || j == 0) {
                                printed = 1;
                                if (digit < 10) {
                                    temp_buffer[len++] = '0' + digit;
                                } else {
                                    temp_buffer[len++] = 'a' + (digit - 10);
                                }
                            }
                        }
                        break;
                    }
                    case 's': {
                        const char* str = va_arg(args, const char*);
                        while (*str && len < 255) {
                            temp_buffer[len++] = *str++;
                        }
                        break;
                    }
                    case '%':
                        temp_buffer[len++] = '%';
                        break;
                    default:
                        temp_buffer[len++] = '%';
                        temp_buffer[len++] = format[i];
                        break;
                }
            } else {
                temp_buffer[len++] = format[i];
            }
        }
        
        temp_buffer[len] = '\0';
        
        // Copy to log buffer
        if (len <= remaining) {
            strcpy(diag_log_buffer + diag_log_pos, temp_buffer);
            diag_log_pos += len;
        } else {
            // Not enough space, truncate
            strncpy(diag_log_buffer + diag_log_pos, temp_buffer, remaining);
            diag_log_pos += remaining;
        }
    }
    
    va_end(args);
}

// Initialize the interrupt diagnostics module
void interrupt_diag_init(void) {
    // Clear log buffer
    memset(diag_log_buffer, 0, DIAG_LOG_SIZE);
    diag_log_pos = 0;
    
    diag_log("Interrupt diagnostics initialized\n");
    terminal_writestring("Interrupt diagnostics initialized\n");
}

// Helper function to read IDTR
static void read_idtr(uint32_t* base, uint16_t* limit) {
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) idtr;
    
    asm volatile("sidt %0" : "=m"(idtr));
    
    *base = idtr.base;
    *limit = idtr.limit;
}

// Helper function to read GDTR
static void read_gdtr(uint32_t* base, uint16_t* limit) {
    struct {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) gdtr;
    
    asm volatile("sgdt %0" : "=m"(gdtr));
    
    *base = gdtr.base;
    *limit = gdtr.limit;
}

// Capture the current CPU state
// Fix for the capture_state function - replace the complex inline assembly

// Capture the current CPU state
void interrupt_diag_capture_state(cpu_state_t* state) {
    // Default initialize to avoid uninitialized values
    state->eax = 0;
    state->ebx = 0;
    state->ecx = 0;
    state->edx = 0;
    state->esi = 0;
    state->edi = 0;
    state->ebp = 0;
    state->esp = 0;
    
    // Capture general registers one by one - safer approach
    asm volatile("movl %%eax, %0" : "=r"(state->eax));
    asm volatile("movl %%ebx, %0" : "=r"(state->ebx));
    asm volatile("movl %%ecx, %0" : "=r"(state->ecx));
    asm volatile("movl %%edx, %0" : "=r"(state->edx));
    asm volatile("movl %%esi, %0" : "=r"(state->esi));
    asm volatile("movl %%edi, %0" : "=r"(state->edi));
    asm volatile("movl %%ebp, %0" : "=r"(state->ebp));
    asm volatile("movl %%esp, %0" : "=r"(state->esp));
    
    // Get EIP (approximate) using label trick
    uint32_t eip;
    asm volatile(
        "call 1f\n"
        "1: pop %0\n"
        : "=r"(eip)
    );
    state->eip = eip;
    
    // Get EFLAGS
    asm volatile(
        "pushf\n"
        "pop %0\n"
        : "=r"(state->eflags)
    );
    
	// Fix for segment register capture - these need special handling

	// Get segment registers using memory operands
	asm volatile("movw %%cs, %0" : "=m"(state->cs));
	asm volatile("movw %%ds, %0" : "=m"(state->ds));
	asm volatile("movw %%es, %0" : "=m"(state->es));
	asm volatile("movw %%fs, %0" : "=m"(state->fs));
	asm volatile("movw %%gs, %0" : "=m"(state->gs));
	asm volatile("movw %%ss, %0" : "=m"(state->ss));
    
    // Get control registers
    asm volatile("movl %%cr0, %0" : "=r"(state->cr0));
    asm volatile("movl %%cr2, %0" : "=r"(state->cr2));
    asm volatile("movl %%cr3, %0" : "=r"(state->cr3));
    
    // Try to get CR4 if available (might cause exception on old CPUs)
    uint32_t cr4 = 0;
    asm volatile("movl %%cr4, %0" : "=r"(cr4));
    state->cr4 = cr4;
    
    // Get descriptor table registers
    uint16_t idt_limit, gdt_limit;
    read_idtr(&state->idt_base, &idt_limit);
    read_gdtr(&state->gdt_base, &gdt_limit);
    state->idt_limit = idt_limit;
    state->gdt_limit = gdt_limit;
    
    // Read PIC state
    
    // Get current masks
    state->pic1_mask = inb(0x21);
    state->pic2_mask = inb(0xA1);
    
    // Read IRR (Interrupt Request Register)
    outb(0x20, 0x0A);  // OCW3 to PIC1: Read IRR
    outb(0xA0, 0x0A);  // OCW3 to PIC2: Read IRR
    state->pic1_irr = inb(0x20);
    state->pic2_irr = inb(0xA0);
    
    // Read ISR (In-Service Register)
    outb(0x20, 0x0B);  // OCW3 to PIC1: Read ISR
    outb(0xA0, 0x0B);  // OCW3 to PIC2: Read ISR
    state->pic1_isr = inb(0x20);
    state->pic2_isr = inb(0xA0);
}

// Print the current CPU state
void interrupt_diag_print_state(const cpu_state_t* state) {
    // Print to both terminal and log
    terminal_writestring("CPU State:\n");
    diag_log("CPU State:\n");
    
    // General registers
    terminal_printf("EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n",
        state->eax, state->ebx, state->ecx, state->edx);
    diag_log("EAX: 0x%x  EBX: 0x%x  ECX: 0x%x  EDX: 0x%x\n",
        state->eax, state->ebx, state->ecx, state->edx);
    
    terminal_printf("ESI: 0x%x  EDI: 0x%x  EBP: 0x%x  ESP: 0x%x\n",
        state->esi, state->edi, state->ebp, state->esp);
    diag_log("ESI: 0x%x  EDI: 0x%x  EBP: 0x%x  ESP: 0x%x\n",
        state->esi, state->edi, state->ebp, state->esp);
    
    terminal_printf("EIP: 0x%x  EFLAGS: 0x%x\n",
        state->eip, state->eflags);
    diag_log("EIP: 0x%x  EFLAGS: 0x%x\n",
        state->eip, state->eflags);
    
    // Segment registers
    terminal_printf("CS: 0x%x  DS: 0x%x  ES: 0x%x  FS: 0x%x  GS: 0x%x  SS: 0x%x\n",
        state->cs, state->ds, state->es, state->fs, state->gs, state->ss);
    diag_log("CS: 0x%x  DS: 0x%x  ES: 0x%x  FS: 0x%x  GS: 0x%x  SS: 0x%x\n",
        state->cs, state->ds, state->es, state->fs, state->gs, state->ss);
    
    // Control registers
    terminal_printf("CR0: 0x%x  CR2: 0x%x  CR3: 0x%x  CR4: 0x%x\n",
        state->cr0, state->cr2, state->cr3, state->cr4);
    diag_log("CR0: 0x%x  CR2: 0x%x  CR3: 0x%x  CR4: 0x%x\n",
        state->cr0, state->cr2, state->cr3, state->cr4);
    
    // Descriptor tables
    terminal_printf("IDTR: Base=0x%x Limit=0x%x\n",
        state->idt_base, state->idt_limit);
    diag_log("IDTR: Base=0x%x Limit=0x%x\n",
        state->idt_base, state->idt_limit);
    
    terminal_printf("GDTR: Base=0x%x Limit=0x%x\n",
        state->gdt_base, state->gdt_limit);
    diag_log("GDTR: Base=0x%x Limit=0x%x\n",
        state->gdt_base, state->gdt_limit);
    
    // PIC state
    terminal_printf("PIC1 Mask: 0x%x  PIC2 Mask: 0x%x\n",
        state->pic1_mask, state->pic2_mask);
    diag_log("PIC1 Mask: 0x%x  PIC2 Mask: 0x%x\n",
        state->pic1_mask, state->pic2_mask);
    
    terminal_printf("PIC1 IRR: 0x%x  PIC2 IRR: 0x%x\n",
        state->pic1_irr, state->pic2_irr);
    diag_log("PIC1 IRR: 0x%x  PIC2 IRR: 0x%x\n",
        state->pic1_irr, state->pic2_irr);
    
    terminal_printf("PIC1 ISR: 0x%x  PIC2 ISR: 0x%x\n",
        state->pic1_isr, state->pic2_isr);
    diag_log("PIC1 ISR: 0x%x  PIC2 ISR: 0x%x\n",
        state->pic1_isr, state->pic2_isr);
}

// Structure for IDT entries
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

// Print information about the IDT
void interrupt_diag_print_idt(void) {
    uint32_t idt_base;
    uint16_t idt_limit;
    read_idtr(&idt_base, &idt_limit);
    
    terminal_printf("IDT: Base=0x%x Limit=0x%x\n", idt_base, idt_limit);
    diag_log("IDT: Base=0x%x Limit=0x%x\n", idt_base, idt_limit);
    
    // Number of entries in IDT
    int num_entries = (idt_limit + 1) / sizeof(idt_entry_t);
    
    terminal_printf("IDT has %d entries\n", num_entries);
    diag_log("IDT has %d entries\n", num_entries);
    
    // Check if IDT is accessible
    if (idt_base == 0 || idt_limit == 0) {
        terminal_writestring("IDT appears to be invalid or not set up\n");
        diag_log("IDT appears to be invalid or not set up\n");
        return;
    }
    
    // Print details of first few entries
    idt_entry_t* idt = (idt_entry_t*)idt_base;
    
    terminal_writestring("First 16 IDT Entries:\n");
    diag_log("First 16 IDT Entries:\n");
    terminal_writestring("IDX | Selector | Type | Address\n");
    diag_log("IDX | Selector | Type | Address\n");
    
    for (int i = 0; i < 16 && i < num_entries; i++) {
        uint32_t handler_addr = idt[i].offset_low | ((uint32_t)idt[i].offset_high << 16);
        terminal_printf("%3d | 0x%04x    | 0x%02x | 0x%08x\n", 
            i, idt[i].selector, idt[i].type_attr, handler_addr);
        diag_log("%3d | 0x%04x    | 0x%02x | 0x%08x\n", 
            i, idt[i].selector, idt[i].type_attr, handler_addr);
    }
}

// Structure for GDT entries
typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

// Print information about the GDT
void interrupt_diag_print_gdt(void) {
    uint32_t gdt_base;
    uint16_t gdt_limit;
    read_gdtr(&gdt_base, &gdt_limit);
    
    terminal_printf("GDT: Base=0x%x Limit=0x%x\n", gdt_base, gdt_limit);
    diag_log("GDT: Base=0x%x Limit=0x%x\n", gdt_base, gdt_limit);
    
    // Number of entries in GDT
    int num_entries = (gdt_limit + 1) / sizeof(gdt_entry_t);
    
    terminal_printf("GDT has %d entries\n", num_entries);
    diag_log("GDT has %d entries\n", num_entries);
    
    // Check if GDT is accessible
    if (gdt_base == 0 || gdt_limit == 0) {
        terminal_writestring("GDT appears to be invalid or not set up\n");
        diag_log("GDT appears to be invalid or not set up\n");
        return;
    }
    
    // Print details of first few entries
    gdt_entry_t* gdt = (gdt_entry_t*)gdt_base;
    
    terminal_writestring("First 8 GDT Entries:\n");
    diag_log("First 8 GDT Entries:\n");
    terminal_writestring("IDX | Base      | Limit     | Access | Granularity\n");
    diag_log("IDX | Base      | Limit     | Access | Granularity\n");
    
    for (int i = 0; i < 8 && i < num_entries; i++) {
        uint32_t base = gdt[i].base_low | ((uint32_t)gdt[i].base_middle << 16) | ((uint32_t)gdt[i].base_high << 24);
        uint32_t limit = gdt[i].limit_low | ((uint32_t)(gdt[i].granularity & 0xF) << 16);
        
        terminal_printf("%3d | 0x%08x | 0x%08x | 0x%02x   | 0x%02x\n",
            i, base, limit, gdt[i].access, gdt[i].granularity);
        diag_log("%3d | 0x%08x | 0x%08x | 0x%02x   | 0x%02x\n",
            i, base, limit, gdt[i].access, gdt[i].granularity);
    }
}

// Print PIC state
void interrupt_diag_print_pic_state(void) {
    // Read PIC masks
    uint8_t pic1_mask = inb(0x21);
    uint8_t pic2_mask = inb(0xA1);
    
    terminal_printf("PIC1 Mask: 0x%x  PIC2 Mask: 0x%x\n", pic1_mask, pic2_mask);
    diag_log("PIC1 Mask: 0x%x  PIC2 Mask: 0x%x\n", pic1_mask, pic2_mask);
    
    // Read Interrupt Request Register
    outb(0x20, 0x0A);  // OCW3 to PIC1: Read IRR
    outb(0xA0, 0x0A);  // OCW3 to PIC2: Read IRR
    uint8_t pic1_irr = inb(0x20);
    uint8_t pic2_irr = inb(0xA0);
    
    terminal_printf("PIC1 IRR: 0x%x  PIC2 IRR: 0x%x\n", pic1_irr, pic2_irr);
    diag_log("PIC1 IRR: 0x%x  PIC2 IRR: 0x%x\n", pic1_irr, pic2_irr);
    
    // Read In-Service Register
    outb(0x20, 0x0B);  // OCW3 to PIC1: Read ISR
    outb(0xA0, 0x0B);  // OCW3 to PIC2: Read ISR
    uint8_t pic1_isr = inb(0x20);
    uint8_t pic2_isr = inb(0xA0);
    
    terminal_printf("PIC1 ISR: 0x%x  PIC2 ISR: 0x%x\n", pic1_isr, pic2_isr);
    diag_log("PIC1 ISR: 0x%x  PIC2 ISR: 0x%x\n", pic1_isr, pic2_isr);
    
    // Print PIC1 individual interrupt status
    terminal_writestring("PIC1 Interrupts (IRQ0-7):\n");
    diag_log("PIC1 Interrupts (IRQ0-7):\n");
    
    for (int i = 0; i < 8; i++) {
        uint8_t mask_bit = (pic1_mask >> i) & 1;
        uint8_t irr_bit = (pic1_irr >> i) & 1;
        uint8_t isr_bit = (pic1_isr >> i) & 1;
        
        terminal_printf("IRQ%d: %s, %s, %s\n", i,
            mask_bit ? "Masked" : "Enabled",
            irr_bit ? "Requested" : "No Request",
            isr_bit ? "In Service" : "Not Servicing");
        diag_log("IRQ%d: %s, %s, %s\n", i,
            mask_bit ? "Masked" : "Enabled",
            irr_bit ? "Requested" : "No Request",
            isr_bit ? "In Service" : "Not Servicing");
    }
    
    // Print PIC2 individual interrupt status
    terminal_writestring("PIC2 Interrupts (IRQ8-15):\n");
    diag_log("PIC2 Interrupts (IRQ8-15):\n");
    
    for (int i = 0; i < 8; i++) {
        uint8_t mask_bit = (pic2_mask >> i) & 1;
        uint8_t irr_bit = (pic2_irr >> i) & 1;
        uint8_t isr_bit = (pic2_isr >> i) & 1;
        
        terminal_printf("IRQ%d: %s, %s, %s\n", i + 8,
            mask_bit ? "Masked" : "Enabled",
            irr_bit ? "Requested" : "No Request",
            isr_bit ? "In Service" : "Not Servicing");
        diag_log("IRQ%d: %s, %s, %s\n", i + 8,
            mask_bit ? "Masked" : "Enabled",
            irr_bit ? "Requested" : "No Request",
            isr_bit ? "In Service" : "Not Servicing");
    }
}

// Test Handler for Exceptions (Interrupts 0-31)
static void test_exception_handler(void) {
    diag_log("Exception handler called\n");
    terminal_writestring("EXCEPTION: Handler called\n");
    
    // Capture CPU state
    cpu_state_t state;
    interrupt_diag_capture_state(&state);
    
    // Print basic info
    terminal_printf("Exception at EIP: 0x%x\n", state.eip);
    diag_log("Exception at EIP: 0x%x\n", state.eip);
    
    // For a proper handler, we would need to handle the interrupt properly,
    // but for diagnostic purposes, we'll just halt
    terminal_writestring("Halting system for diagnostic analysis\n");
    diag_log("Halting system for diagnostic analysis\n");
    
    // Infinite loop
    for (;;) {
        asm volatile("hlt");
    }
}

// Test Handler for IRQs (Interrupts 32-47)
static void test_irq_handler(void) {
    diag_log("IRQ handler called\n");
    terminal_writestring("IRQ: Handler called\n");
    
    // Send EOI (End of Interrupt) to PIC
    // For IRQ0-7
    outb(0x20, 0x20);
    
    // If it's IRQ8-15, also send EOI to slave PIC
    // outb(0xA0, 0x20);
    
    // Get CPU state
    cpu_state_t state;
    interrupt_diag_capture_state(&state);
    
    // Print interrupt details
    terminal_printf("IRQ handler called, EIP: 0x%x\n", state.eip);
    diag_log("IRQ handler called, EIP: 0x%x\n", state.eip);
    
    // Print PIC state
    interrupt_diag_print_pic_state();
}

// Install test handlers for essential interrupts
void interrupt_diag_install_test_handlers(void) {
    // Placeholder - in a real implementation, we would set up the IDT
    // and install handlers for each interrupt
    diag_log("Test handlers installed (placeholder)\n");
    terminal_writestring("Test interrupt handlers installed\n");
    
    // This is just a placeholder. In a real implementation,
    // we would need to set up the IDT entries for each handler
}

// Enable a single interrupt for testing
void interrupt_diag_enable_single_interrupt(uint8_t irq) {
    diag_log("Enabling IRQ %d for testing\n", irq);
    terminal_printf("Enabling IRQ %d for testing\n", irq);
    
    // Determine which PIC owns this IRQ
    if (irq < 8) {
        // IRQ0-7 are on master PIC
        uint8_t mask = inb(0x21);
        mask &= ~(1 << irq); // Clear the bit to enable the IRQ
        outb(0x21, mask);
        
        diag_log("Updated PIC1 mask: 0x%x\n", inb(0x21));
        terminal_printf("Updated PIC1 mask: 0x%x\n", inb(0x21));
    } else if (irq < 16) {
        // IRQ8-15 are on slave PIC
        // First ensure IRQ2 is enabled on master (cascade)
        uint8_t master_mask = inb(0x21);
        master_mask &= ~(1 << 2); // Clear bit 2 to enable IRQ2 (cascade)
        outb(0x21, master_mask);
        
        // Now enable the specific IRQ on slave
        uint8_t slave_mask = inb(0xA1);
        slave_mask &= ~(1 << (irq - 8)); // Clear the bit to enable the IRQ
        outb(0xA1, slave_mask);
        
        diag_log("Updated PIC1 mask: 0x%x\n", inb(0x21));
        diag_log("Updated PIC2 mask: 0x%x\n", inb(0xA1));
        
        terminal_printf("Updated PIC1 mask: 0x%x\n", inb(0x21));
        terminal_printf("Updated PIC2 mask: 0x%x\n", inb(0xA1));
    } else {
        diag_log("Invalid IRQ number: %d\n", irq);
        terminal_printf("Invalid IRQ number: %d\n", irq);
    }
    
    // Enable interrupts globally
    diag_log("Enabling interrupts globally (STI)\n");
    terminal_writestring("Enabling interrupts globally (STI)\n");
    
    asm volatile("sti");
}

// Test interrupts with diagnostics (safe mode)
void interrupt_diag_test_interrupts(void) {
    diag_log("Starting interrupt diagnostics test\n");
    terminal_writestring("Starting interrupt diagnostics test\n");
    
    // Capture initial CPU state
    cpu_state_t initial_state;
    interrupt_diag_capture_state(&initial_state);
    
    // Print initial state
    terminal_writestring("Initial CPU state:\n");
    interrupt_diag_print_state(&initial_state);
    
    // Print IDT and GDT information
    interrupt_diag_print_idt();
    interrupt_diag_print_gdt();
    
    // Print PIC state
    interrupt_diag_print_pic_state();
    
    // Test PIC initialization
    terminal_writestring("Testing PIC initialization...\n");
    diag_log("Testing PIC initialization...\n");
    
    // Initialize master PIC
    outb(0x20, 0x11); // ICW1: Initialize + ICW4 needed
    outb(0x21, 0x20); // ICW2: IRQ0-7 mapped to interrupts 32-39
    outb(0x21, 0x04); // ICW3: IRQ2 has a slave
	outb(0x21, 0x01); // ICW4: 8086 mode
    
    // Initialize slave PIC
    outb(0xA0, 0x11); // ICW1: Initialize + ICW4 needed
    outb(0xA1, 0x28); // ICW2: IRQ8-15 mapped to interrupts 40-47
    outb(0xA1, 0x02); // ICW3: This PIC is a slave on IRQ2
    outb(0xA1, 0x01); // ICW4: 8086 mode
    
    // Mask all interrupts initially
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    
    // Print PIC state after initialization
    terminal_writestring("PIC state after initialization:\n");
    diag_log("PIC state after initialization:\n");
    interrupt_diag_print_pic_state();
    
    // Perform an IDT setup test
    terminal_writestring("Testing IDT setup...\n");
    diag_log("Testing IDT setup...\n");
    
    // Create a minimal IDT with just enough entries for basic functionality
    // This is a simplified version for diagnosis only
    struct idt_entry {
        uint16_t offset_low;
        uint16_t selector;
        uint8_t zero;
        uint8_t type_attr;
        uint16_t offset_high;
    } __attribute__((packed));
    
    struct idt_ptr {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed));
    
    // Allocate space for minimal IDT (48 entries: 32 exceptions + 16 IRQs)
    struct idt_entry test_idt[48] __attribute__((aligned(8)));
    
    // Fill IDT with default handlers
    for (int i = 0; i < 48; i++) {
        // Use the same handler for all interrupts in this test
        uint32_t handler = (uint32_t)test_exception_handler;
        
        // For IRQs, use the IRQ handler
        if (i >= 32) {
            handler = (uint32_t)test_irq_handler;
        }
        
        test_idt[i].offset_low = handler & 0xFFFF;
        test_idt[i].offset_high = (handler >> 16) & 0xFFFF;
        test_idt[i].selector = 0x08; // Kernel code segment
        test_idt[i].zero = 0;
        test_idt[i].type_attr = 0x8E; // Present, Ring 0, 32-bit Interrupt Gate
    }
    
    // Create IDT pointer
    struct idt_ptr test_idtp;
    test_idtp.limit = (sizeof(struct idt_entry) * 48) - 1;
    test_idtp.base = (uint32_t)&test_idt;
    
    // Print test IDT information
    terminal_printf("Test IDT: Base=0x%x Limit=0x%x\n", test_idtp.base, test_idtp.limit);
    diag_log("Test IDT: Base=0x%x Limit=0x%x\n", test_idtp.base, test_idtp.limit);
    
    // Load the test IDT
    terminal_writestring("Loading test IDT...\n");
    diag_log("Loading test IDT...\n");
    
    asm volatile("lidt %0" : : "m"(test_idtp));
    
    // Read back IDT to verify it was loaded
    uint32_t idt_base;
    uint16_t idt_limit;
    read_idtr(&idt_base, &idt_limit);
    
    terminal_printf("Loaded IDT: Base=0x%x Limit=0x%x\n", idt_base, idt_limit);
    diag_log("Loaded IDT: Base=0x%x Limit=0x%x\n", idt_base, idt_limit);
    
    // Test enabling a single interrupt (timer IRQ0)
    terminal_writestring("Testing timer IRQ (0)...\n");
    diag_log("Testing timer IRQ (0)...\n");
    
    // Enable interrupts and timer IRQ
    interrupt_diag_enable_single_interrupt(0);
    
    // Wait a bit for interrupts to occur
    terminal_writestring("Waiting for timer interrupts...\n");
    diag_log("Waiting for timer interrupts...\n");
    
    // Simple delay
    for (volatile int i = 0; i < 10000000; i++) {
        // Do nothing, just wait
    }
    
    // Disable interrupts
    asm volatile("cli");
    
    // Check PIC state after test
    terminal_writestring("PIC state after test:\n");
    diag_log("PIC state after test:\n");
    interrupt_diag_print_pic_state();
    
    // Capture final CPU state
    cpu_state_t final_state;
    interrupt_diag_capture_state(&final_state);
    
    // Print final state
    terminal_writestring("Final CPU state:\n");
    interrupt_diag_print_state(&final_state);
    
    // Test complete
    terminal_writestring("Interrupt diagnostic test complete\n");
    diag_log("Interrupt diagnostic test complete\n");
}

// Get the diagnostics log buffer
const char* interrupt_diag_get_log(void) {
    return diag_log_buffer;
}