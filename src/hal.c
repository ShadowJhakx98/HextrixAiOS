// src/hal.c
#include "hal.h"
#include "terminal.h"
#include "string.h"

// Device registry
#define MAX_DEVICES 16
static hal_device_t* devices[MAX_DEVICES] = {0};
static int num_devices = 0;

// HAL initialization
void hal_init(void) {
    terminal_writestring("Initializing Hardware Abstraction Layer (HAL)\n");
    
    // Clear device registry
    for (int i = 0; i < MAX_DEVICES; i++) {
        devices[i] = 0;
    }
    
    num_devices = 0;
    
    terminal_writestring("HAL initialized in polling mode\n");
}

// Register a device with HAL
int hal_register_device(hal_device_t* device) {
    if (!device || num_devices >= MAX_DEVICES) {
        return -1; // Invalid device or registry full
    }
    
    // Add device to registry
    devices[num_devices++] = device;
    
    // Initialize the device
    if (device->init) {
        return device->init(device);
    }
    
    return 0;
}

// Get a device by type
hal_device_t* hal_get_device(uint32_t type) {
    for (int i = 0; i < num_devices; i++) {
        if (devices[i] && devices[i]->type == type) {
            return devices[i];
        }
    }
    
    return 0; // Device not found
}

// Forward declarations for device initializers
extern int hal_timer_init(void);
extern int hal_keyboard_init(void);
extern int hal_framebuffer_init(void);
extern int hal_mouse_init(void);

// Initialize all HAL devices
int hal_init_devices(void) {
    int status = 0;
    
    // Initialize framebuffer first (needed for GUI)
    status = hal_framebuffer_init();
    if (status != 0) {
        terminal_writestring("Failed to initialize HAL framebuffer device\n");
        return status;
    }
    
    // Initialize mouse (needed for GUI interaction)
    status = hal_mouse_init();
    if (status != 0) {
        terminal_writestring("Failed to initialize HAL mouse device\n");
        // Continue even if mouse fails - GUI might still work with keyboard only
    }
    
    // Initialize timer
    status = hal_timer_init();
    if (status != 0) {
        terminal_writestring("Failed to initialize HAL timer device\n");
        return status;
    }
    
    // Initialize keyboard
    status = hal_keyboard_init();
    if (status != 0) {
        terminal_writestring("Failed to initialize HAL keyboard device\n");
        return status;
    }
    
    terminal_writestring("All HAL devices initialized successfully\n");
    return 0;
}

// Generic device operations wrapper functions

int hal_device_read(hal_device_t* device, void* buffer, uint32_t size) {
    if (!device || !device->read) {
        return -1;
    }
    return device->read(device, buffer, size);
}

int hal_device_write(hal_device_t* device, const void* buffer, uint32_t size) {
    if (!device || !device->write) {
        return -1;
    }
    return device->write(device, buffer, size);
}

int hal_device_ioctl(hal_device_t* device, uint32_t request, void* arg) {
    if (!device || !device->ioctl) {
        return -1;
    }
    return device->ioctl(device, request, arg);
}

int hal_device_close(hal_device_t* device) {
    if (!device || !device->close) {
        return -1;
    }
    return device->close(device);
}