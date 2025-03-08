// src/hal.c - add or modify to include hal_init_devices
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

// Initialize all HAL devices
int hal_init_devices(void) {
    int status = 0;
    
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
    
    return 0;
}