#include <stdint.h>
#include "terminal.h"

enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | (bg << 4);
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | ((uint16_t)color << 8);
}

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*)0xB8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

// Add this function to clear the terminal screen
void terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
}

void terminal_putchar(char c) {
    unsigned char uc = c;
    
    // Handle special characters
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
        }
        return;
    } else if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] = vga_entry(' ', terminal_color);
        } else if (terminal_row > 0) {
            terminal_row--;
            terminal_column = VGA_WIDTH - 1;
            terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] = vga_entry(' ', terminal_color);
        }
        return;
    }
    
    terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] = vga_entry(uc, terminal_color);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll();
        }
    }
}

// Add scrolling function to terminal.c
void terminal_scroll(void) {
    // Move each line up one position
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t dst_index = y * VGA_WIDTH + x;
            const size_t src_index = (y + 1) * VGA_WIDTH + x;
            terminal_buffer[dst_index] = terminal_buffer[src_index];
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
    
    // Set cursor to start of last line
    terminal_row = VGA_HEIGHT - 1;
}

void terminal_writestring(const char* data) {
    for (size_t i = 0; data[i] != '\0'; i++)
        terminal_putchar(data[i]);
}