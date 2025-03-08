// src/hal_display.c
#include "hal.h"
#include "terminal.h"
#include "stdio.h"
#include <stdint.h>

// Display device private data
typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t color;
    uint16_t cursor_x;
    uint16_t cursor_y;
    uint16_t* buffer;
} display_data_t;

// VGA color constants
#define VGA_COLOR_BLACK         0
#define VGA_COLOR_BLUE          1
#define VGA_COLOR_GREEN         2
#define VGA_COLOR_CYAN          3
#define VGA_COLOR_RED           4
#define VGA_COLOR_MAGENTA       5
#define VGA_COLOR_BROWN         6
#define VGA_COLOR_LIGHT_GREY    7
#define VGA_COLOR_DARK_GREY     8
#define VGA_COLOR_LIGHT_BLUE    9
#define VGA_COLOR_LIGHT_GREEN   10
#define VGA_COLOR_LIGHT_CYAN    11
#define VGA_COLOR_LIGHT_RED     12
#define VGA_COLOR_LIGHT_MAGENTA 13
#define VGA_COLOR_LIGHT_BROWN   14
#define VGA_COLOR_YELLOW        14  // Same as LIGHT_BROWN in VGA palette
#define VGA_COLOR_WHITE         15

// Display modes
#define DISPLAY_MODE_TEXT       0
#define DISPLAY_MODE_GRAPHICS   1

// Local display device
static display_data_t display_data = {0};
static hal_device_t display_device = {0};

// Helper functions
static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | (bg << 4);
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t)uc | ((uint16_t)color << 8);
}

// Scroll the display
static void display_scroll(void) {
    display_data_t* data = &display_data;
    
    // Move each line up one position
    for (uint16_t y = 0; y < data->height - 1; y++) {
        for (uint16_t x = 0; x < data->width; x++) {
            const uint16_t dst_index = y * data->width + x;
            const uint16_t src_index = (y + 1) * data->width + x;
            data->buffer[dst_index] = data->buffer[src_index];
        }
    }
    
    // Clear the last line
    for (uint16_t x = 0; x < data->width; x++) {
        const uint16_t index = (data->height - 1) * data->width + x;
        data->buffer[index] = vga_entry(' ', data->color);
    }
    
    // Set cursor to start of last line
    data->cursor_y = data->height - 1;
    data->cursor_x = 0;
}

// Write a character at the current cursor position
static void display_putchar_at(char c, uint8_t color, uint16_t x, uint16_t y) {
    display_data_t* data = &display_data;
    
    const uint16_t index = y * data->width + x;
    data->buffer[index] = vga_entry(c, color);
}

// Write a character with current color and advance cursor
static void display_putchar(char c) {
    display_data_t* data = &display_data;
    
    // Handle special characters
    if (c == '\n') {
        // Newline
        data->cursor_x = 0;
        if (++data->cursor_y == data->height) {
            display_scroll();
        }
        return;
    } else if (c == '\r') {
        // Carriage return
        data->cursor_x = 0;
        return;
    } else if (c == '\t') {
        // Tab (advance to next 8-character boundary)
        data->cursor_x = (data->cursor_x + 8) & ~7;
        if (data->cursor_x >= data->width) {
            data->cursor_x = 0;
            if (++data->cursor_y == data->height) {
                display_scroll();
            }
        }
        return;
    } else if (c == '\b') {
        // Backspace
        if (data->cursor_x > 0) {
            data->cursor_x--;
            display_putchar_at(' ', data->color, data->cursor_x, data->cursor_y);
        } else if (data->cursor_y > 0) {
            data->cursor_y--;
            data->cursor_x = data->width - 1;
            display_putchar_at(' ', data->color, data->cursor_x, data->cursor_y);
        }
        return;
    }
    
    // Write the character and advance cursor
    display_putchar_at(c, data->color, data->cursor_x, data->cursor_y);
    if (++data->cursor_x == data->width) {
        data->cursor_x = 0;
        if (++data->cursor_y == data->height) {
            display_scroll();
        }
    }
}

// Write a string to the display
static void display_write(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        display_putchar(str[i]);
    }
}

// Clear the display
static void display_clear(void) {
    display_data_t* data = &display_data;
    
    // Clear all characters
    for (uint16_t y = 0; y < data->height; y++) {
        for (uint16_t x = 0; x < data->width; x++) {
            const uint16_t index = y * data->width + x;
            data->buffer[index] = vga_entry(' ', data->color);
        }
    }
    
    // Reset cursor
    data->cursor_x = 0;
    data->cursor_y = 0;
}

// Set cursor position
static void display_set_cursor(uint16_t x, uint16_t y) {
    display_data_t* data = &display_data;
    
    // Validate position
    if (x >= data->width) {
        x = data->width - 1;
    }
    if (y >= data->height) {
        y = data->height - 1;
    }
    
    // Set cursor position
    data->cursor_x = x;
    data->cursor_y = y;
}

// Set display color
static void display_set_color(uint8_t fg, uint8_t bg) {
    display_data_t* data = &display_data;
    data->color = vga_entry_color(fg, bg);
}

// Device-specific functions
static int display_init(void* device) {
    hal_device_t* dev = (hal_device_t*)device;
    display_data_t* data = (display_data_t*)dev->private_data;
    
    // Initialize display data
    data->width = 80;
    data->height = 25;
    data->color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    data->cursor_x = 0;
    data->cursor_y = 0;
    data->buffer = (uint16_t*)0xB8000; // VGA text mode buffer
    
    // Clear the display
    display_clear();
    
    terminal_writestring("HAL Display initialized in polling mode\n");
    
    return 0;
}

static int display_close(void* device) {
    // Nothing to do for VGA text mode
    return 0;
}

static int display_read(void* device, void* buffer, uint32_t size) {
    // Display is write-only
    return -1;
}

static int display_write(void* device, const void* buffer, uint32_t size) {
    hal_device_t* dev = (hal_device_t*)device;
    display_data_t* data = (display_data_t*)dev->private_data;
    
    // Write characters to display
    const char* str = (const char*)buffer;
    for (uint32_t i = 0; i < size; i++) {
        display_putchar(str[i]);
    }
    
    return size;
}

static int display_ioctl(void* device, uint32_t request, void* arg) {
    hal_device_t* dev = (hal_device_t*)device;
    display_data_t* data = (display_data_t*)dev->private_data;
    
    switch (request) {
        case 0: // Get display dimensions
            if (arg) {
                uint32_t* dims = (uint32_t*)arg;
                dims[0] = data->width;
                dims[1] = data->height;
                return 0;
            }
            break;
            
        case 1: // Set cursor position
            if (arg) {
                uint32_t* pos = (uint32_t*)arg;
                display_set_cursor(pos[0], pos[1]);
                return 0;
            }
            break;
            
        case 2: // Set color
            if (arg) {
                uint32_t* colors = (uint32_t*)arg;
                display_set_color(colors[0], colors[1]);
                return 0;
            }
            break;
            
        case 3: // Clear display
            display_clear();
            return 0;
    }
    
    return -1;
}

// HAL display interface functions
void hal_display_putchar(char c) {
    display_putchar(c);
}

void hal_display_writestring(const char* str) {
    display_write(str);
}

void hal_display_clear(void) {
    display_clear();
}

void hal_display_set_cursor(uint16_t x, uint16_t y) {
    display_set_cursor(x, y);
}

void hal_display_set_color(uint8_t fg, uint8_t bg) {
    display_set_color(fg, bg);
}

void hal_display_get_dimensions(uint16_t* width, uint16_t* height) {
    if (width) *width = display_data.width;
    if (height) *height = display_data.height;
}

// Initialize and register display device
int hal_display_init(void) {
    // Setup device
    display_device.type = HAL_DEVICE_DISPLAY;
    display_device.mode = HAL_MODE_POLLING;
    display_device.private_data = &display_data;
    
    // Set functions
    display_device.init = display_init;
    display_device.close = display_close;
    display_device.read = display_read;
    display_device.write = display_write;
    display_device.ioctl = display_ioctl;
    
    // Register with HAL
    return hal_register_device(&display_device);
}

// Enhanced console features

// Draw a box with specified dimensions
void hal_display_draw_box(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    display_data_t* data = &display_data;
    uint8_t saved_color = data->color;
    
    // Check boundaries
    if (x + width > data->width) width = data->width - x;
    if (y + height > data->height) height = data->height - y;
    
    // Draw top border
    display_set_cursor(x, y);
    display_putchar_at('+', data->color, x, y);
    for (uint16_t i = 1; i < width - 1; i++) {
        display_putchar_at('-', data->color, x + i, y);
    }
    if (width > 1) {
        display_putchar_at('+', data->color, x + width - 1, y);
    }
    
    // Draw sides
    for (uint16_t j = 1; j < height - 1; j++) {
        display_putchar_at('|', data->color, x, y + j);
        for (uint16_t i = 1; i < width - 1; i++) {
            display_putchar_at(' ', data->color, x + i, y + j);
        }
        if (width > 1) {
            display_putchar_at('|', data->color, x + width - 1, y + j);
        }
    }
    
    // Draw bottom border
    if (height > 1) {
        display_putchar_at('+', data->color, x, y + height - 1);
        for (uint16_t i = 1; i < width - 1; i++) {
            display_putchar_at('-', data->color, x + i, y + height - 1);
        }
        if (width > 1) {
            display_putchar_at('+', data->color, x + width - 1, y + height - 1);
        }
    }
    
    // Restore cursor position and color
    data->color = saved_color;
}

// Print text inside a box
void hal_display_message_box(const char* title, const char* message) {
    display_data_t* data = &display_data;
    
    // Calculate box dimensions
    uint16_t title_len = 0;
    while (title[title_len]) title_len++;
    
    uint16_t msg_len = 0;
    uint16_t max_line_len = 0;
    uint16_t line_len = 0;
    uint16_t num_lines = 1;
    
    // Calculate message dimensions
    while (message[msg_len]) {
        if (message[msg_len] == '\n') {
            if (line_len > max_line_len) {
                max_line_len = line_len;
            }
            line_len = 0;
            num_lines++;
        } else {
            line_len++;
        }
        msg_len++;
    }
    
    if (line_len > max_line_len) {
        max_line_len = line_len;
    }
    
    // Calculate box position and size
    uint16_t box_width = max_line_len + 4;
    if (title_len + 4 > box_width) {
        box_width = title_len + 4;
    }
    
    uint16_t box_height = num_lines + 4;
    
    uint16_t box_x = (data->width - box_width) / 2;
    uint16_t box_y = (data->height - box_height) / 2;
    
    // Save current color
    uint8_t saved_color = data->color;
    
    // Set color for box
    display_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    
    // Draw box
    hal_display_draw_box(box_x, box_y, box_width, box_height);
    
    // Draw title
    display_set_cursor(box_x + 2, box_y);
    display_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLUE);
    for (uint16_t i = 0; i < title_len; i++) {
        display_putchar_at(title[i], data->color, box_x + 2 + i, box_y);
    }
    
    // Draw message
    display_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    uint16_t cur_x = box_x + 2;
    uint16_t cur_y = box_y + 2;
    
    for (uint16_t i = 0; i < msg_len; i++) {
        if (message[i] == '\n') {
            cur_x = box_x + 2;
            cur_y++;
        } else {
            display_putchar_at(message[i], data->color, cur_x, cur_y);
            cur_x++;
        }
    }
    
    // Restore color
    data->color = saved_color;
}

// Draw a progress bar
void hal_display_progress_bar(uint16_t x, uint16_t y, uint16_t width, 
	uint8_t percent, const char* label) {
display_data_t* data = &display_data;
uint8_t saved_color = data->color;

// Check boundaries
if (x + width > data->width) width = data->width - x;

// Calculate fill width
uint16_t fill_width = (width * percent) / 100;
if (fill_width > width) fill_width = width;

// Initialize label_len outside the if block
uint16_t label_len = 0;

// Draw label
if (label) {
// Calculate label length
while (label[label_len]) label_len++;

if (label_len <= width) {
uint16_t label_x = x + (width - label_len) / 2;
for (uint16_t i = 0; i < label_len; i++) {
if (label_x + i < x + fill_width) {
// Label inside filled area
display_putchar_at(label[i], vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_GREEN), 
		  label_x + i, y);
} else {
// Label in unfilled area
display_putchar_at(label[i], vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY), 
		  label_x + i, y);
}
}
}
}

// Draw border
display_putchar_at('[', saved_color, x, y);
display_putchar_at(']', saved_color, x + width + 1, y);

// Draw filled portion
for (uint16_t i = 0; i < fill_width; i++) {
if (!label || (i < (width - label_len) / 2) || 
(i >= (width - label_len) / 2 + label_len)) {
display_putchar_at('=', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_GREEN), 
  x + 1 + i, y);
}
}

// Draw unfilled portion
for (uint16_t i = fill_width; i < width; i++) {
if (!label || (i < (width - label_len) / 2) || 
(i >= (width - label_len) / 2 + label_len)) {
display_putchar_at(' ', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_DARK_GREY), 
  x + 1 + i, y);
}
}

// Restore color
data->color = saved_color;
}

// Display enhanced status bar at bottom of screen
void hal_display_status_bar(const char* left_text, const char* center_text, const char* right_text) {
    display_data_t* data = &display_data;
    uint8_t saved_color = data->color;
    uint16_t saved_x = data->cursor_x;
    uint16_t saved_y = data->cursor_y;
    
    // Set status bar color
    display_set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    
    // Fill bottom line with spaces
    for (uint16_t i = 0; i < data->width; i++) {
        display_putchar_at(' ', data->color, i, data->height - 1);
    }
    
    // Draw left text
    if (left_text) {
        uint16_t len = 0;
        while (left_text[len] && len < data->width / 3) len++;
        
        for (uint16_t i = 0; i < len; i++) {
            display_putchar_at(left_text[i], data->color, 1 + i, data->height - 1);
        }
    }
    
    // Draw center text
    if (center_text) {
        uint16_t len = 0;
        while (center_text[len] && len < data->width / 3) len++;
        
        uint16_t start = (data->width - len) / 2;
        for (uint16_t i = 0; i < len; i++) {
            display_putchar_at(center_text[i], data->color, start + i, data->height - 1);
        }
    }
    
    // Draw right text
    if (right_text) {
        uint16_t len = 0;
        while (right_text[len] && len < data->width / 3) len++;
        
        uint16_t start = data->width - len - 1;
        for (uint16_t i = 0; i < len; i++) {
            display_putchar_at(right_text[i], data->color, start + i, data->height - 1);
        }
    }
    
    // Restore cursor position and color
    data->color = saved_color;
    data->cursor_x = saved_x;
    data->cursor_y = saved_y;
}