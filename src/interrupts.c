#include <stdint.h>
#include "interrupts.h"
#include "terminal.h"

static void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

struct idt_entry {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_mid;
    uint32_t base_hi;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr idtp;

volatile uint32_t timer_ticks = 0;
volatile char keyboard_buffer[256];
volatile int keyboard_index = 0;

static char scancode_to_ascii(uint8_t scancode) {
    static const char map[] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0, 0,
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
        'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
    };
    if (scancode < sizeof(map)) return map[scancode];
    return 0;
}

void timer_handler(void) {
    timer_ticks++;
    outb(0x20, 0x20); // Send End of Interrupt (EOI) to PIC1
}

void keyboard_handler(void) {
    uint8_t scancode = inb(0x60);
    char c = scancode_to_ascii(scancode);
    if (c && keyboard_index < 255) {
        keyboard_buffer[keyboard_index++] = c;
    }
    outb(0x20, 0x20); // Send EOI to PIC1
}

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_lo = base & 0xFFFF;
    idt[num].base_mid = (base >> 16) & 0xFFFF;
    idt[num].base_hi = (base >> 32) & 0xFFFFFFFF;
    idt[num].sel = sel;
    idt[num].zero = 0;
    idt[num].flags = flags;
    idt[num].reserved = 0;
}

void init_interrupts(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint64_t)&idt;

    // Set up timer (IRQ0) and keyboard (IRQ1) handlers
    idt_set_gate(0x20, (uint64_t)timer_handler, 0x08, 0x8E);
    idt_set_gate(0x21, (uint64_t)keyboard_handler, 0x08, 0x8E);

    // Remap PIC
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x00); outb(0xA1, 0x00);

    // Load IDT and enable interrupts
    asm volatile("lidt %0" : : "m"(idtp));
    asm volatile("sti");
}