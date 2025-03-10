// src/hal_keyboard.c
#include "hal.h"
#include "terminal.h"

// Keyboard device private data
typedef struct {
    int last_scancode;
    int buffer[16];
    int buffer_head;
    int buffer_tail;
} keyboard_data_t;

// Local keyboard device
static keyboard_data_t keyboard_data = {0};
static hal_device_t keyboard_device = {0};

// Device-specific functions
static int keyboard_init(void* device) {
    hal_device_t* dev = (hal_device_t*)device;
    keyboard_data_t* data = (keyboard_data_t*)dev->private_data;
    
    // Initialize data
    data->last_scancode = 0;
    data->buffer_head = 0;
    data->buffer_tail = 0;
    
    terminal_writestring("HAL Keyboard initialized in polling mode\n");
    
    return 0;
}

static int keyboard_close(void* device) {
    return 0;
}

static int keyboard_read(void* device, void* buffer, uint32_t size) {
    hal_device_t* dev = (hal_device_t*)device;
    keyboard_data_t* data = (keyboard_data_t*)dev->private_data;
    
    if (size < sizeof(int)) {
        return -1;
    }
    
    // Read scancode from buffer if available
    if (data->buffer_head != data->buffer_tail) {
        *((int*)buffer) = data->buffer[data->buffer_tail];
        data->buffer_tail = (data->buffer_tail + 1) % 16;
        return sizeof(int);
    }
    
    return 0; // No data available
}

static int keyboard_write(void* device, const void* buffer, uint32_t size) {
    return -1; // Keyboard is read-only
}

static int keyboard_ioctl(void* device, uint32_t request, void* arg) {
    return -1; // No ioctls defined yet
}

// Poll keyboard for input
void hal_keyboard_poll(void) {
    keyboard_data_t* data = &keyboard_data;
    
    // Check if keyboard has data
    if (inb(0x64) & 1) {
        int scancode = inb(0x60);
        
        // Store in buffer if space available
        int next_head = (data->buffer_head + 1) % 16;
        if (next_head != data->buffer_tail) {
            data->buffer[data->buffer_head] = scancode;
            data->buffer_head = next_head;
        }
        
        data->last_scancode = scancode;
    }
}

// HAL keyboard interface functions
int hal_keyboard_read(void) {
    keyboard_data_t* data = &keyboard_data;
    
    // Poll keyboard first
    hal_keyboard_poll();
    
    // Read from buffer if available
    if (data->buffer_head != data->buffer_tail) {
        int scancode = data->buffer[data->buffer_tail];
        data->buffer_tail = (data->buffer_tail + 1) % 16;
        return scancode;
    }
    
    return -1; // No key available
}

int hal_keyboard_is_key_available(void) {
    keyboard_data_t* data = &keyboard_data;
    
    // Poll keyboard first
    hal_keyboard_poll();
    
    return (data->buffer_head != data->buffer_tail);
}

// Initialize and register keyboard device
int hal_keyboard_init(void) {
    // Setup device
    keyboard_device.type = HAL_DEVICE_KEYBOARD;
    keyboard_device.mode = HAL_MODE_POLLING;
    keyboard_device.private_data = &keyboard_data;
    
    // Set functions
    keyboard_device.init = keyboard_init;
    keyboard_device.close = keyboard_close;
    keyboard_device.read = keyboard_read;
    keyboard_device.write = keyboard_write;
    keyboard_device.ioctl = keyboard_ioctl;
    
    // Register with HAL
    return hal_register_device(&keyboard_device);
}