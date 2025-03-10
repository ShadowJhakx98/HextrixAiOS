// src/hal.c
#include "hal.h"
#include "terminal.h"
#include "string.h"

// Serial debug function declarations
void serial_print(const char* str);
#define SERIAL_DEBUG(msg) serial_print(msg)

// Function to print hex values for debugging
void print_hex(unsigned int num) {
    static const char hex_chars[] = "0123456789ABCDEF";
    char output[11]; // 0x + 8 digits + null terminator
    
    output[0] = '0';
    output[1] = 'x';
    
    for (int i = 0; i < 8; i++) {
        output[2 + i] = hex_chars[(num >> (28 - i * 4)) & 0xF];
    }
    
    output[10] = '\0';
    serial_print(output);
}

// Note: The following declarations should ideally be in hal.h to ensure proper linkage
// across all files that use these functions (e.g., kernel.c).
void outb(unsigned short port, unsigned char value);
unsigned char inb(unsigned short port);
int hal_framebuffer_is_ready(void);
int hal_is_system_stable(void);

// Device registry
#define MAX_DEVICES 16
static hal_device_t* devices[MAX_DEVICES] = {0};
static int num_devices = 0;

// HAL initialization
int hal_init(void) {
    SERIAL_DEBUG("Starting HAL initialization...\n");
    terminal_writestring("Initializing Hardware Abstraction Layer (HAL)\n");
    
    // Clear device registry
    SERIAL_DEBUG("Clearing device registry...\n");
    for (int i = 0; i < MAX_DEVICES; i++) {
        devices[i] = 0;
    }
    
    num_devices = 0;
    
    SERIAL_DEBUG("HAL base initialization complete\n");
    terminal_writestring("HAL initialized in polling mode\n");
    return 0;  // Success
}

// Port I/O functions
void outb(unsigned short port, unsigned char value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

unsigned char inb(unsigned short port) {
    unsigned char value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Stub functions (to be replaced with actual implementations)
int hal_framebuffer_is_ready(void) {
    SERIAL_DEBUG("Checking framebuffer readiness...\n");
    // Stub: Replace with actual framebuffer readiness check
    return 1;  // Assume ready for now
}

int hal_is_system_stable(void) {
    // Stub: Replace with actual system stability check
    return 1;  // Assume stable for now
}

// Register a device with HAL
int hal_register_device(hal_device_t* device) {
    SERIAL_DEBUG("Registering device: ");
    print_hex((unsigned int)device);
    SERIAL_DEBUG("\n");
    
    if (!device) {
        SERIAL_DEBUG("ERROR: Null device pointer!\n");
        return -1;
    }
    
    if (num_devices >= MAX_DEVICES) {
        SERIAL_DEBUG("ERROR: Device registry full!\n");
        return -1;
    }
    
    // Add device to registry
    devices[num_devices++] = device;
    SERIAL_DEBUG("Device added to registry at index: ");
    print_hex(num_devices - 1);
    SERIAL_DEBUG("\n");
    
    // Initialize the device
    if (device->init) {
        SERIAL_DEBUG("Calling device init function\n");
        int result = device->init(device);
        SERIAL_DEBUG("Device init returned: ");
        print_hex(result);
        SERIAL_DEBUG("\n");
        return result;
    }
    
    SERIAL_DEBUG("Device has no init function\n");
    return 0;
}

// Get a device by type
hal_device_t* hal_get_device(uint32_t type) {
    SERIAL_DEBUG("Looking for device type: ");
    print_hex(type);
    SERIAL_DEBUG("\n");
    
    for (int i = 0; i < num_devices; i++) {
        if (devices[i] && devices[i]->type == type) {
            SERIAL_DEBUG("Device found at index: ");
            print_hex(i);
            SERIAL_DEBUG("\n");
            return devices[i];
        }
    }
    
    SERIAL_DEBUG("Device not found\n");
    return 0; // Device not found
}

// Forward declarations for device initializers
extern int hal_timer_init(void);
extern int hal_keyboard_init(void);
extern int hal_framebuffer_init(void);
extern int hal_mouse_init(void);

int hal_init_devices(void) {
    int status = 0;
    SERIAL_DEBUG("Starting HAL device initialization...\n");

    // Initialize framebuffer first (needed for GUI)
    SERIAL_DEBUG("Initializing framebuffer...\n");
    hal_device_t* fb_device = framebuffer_init();
    if (fb_device == NULL) {
        SERIAL_DEBUG("Failed to initialize framebuffer\n");
        terminal_writestring("Failed to initialize HAL framebuffer device\n");
        return -1;  // Or another appropriate error code
    }
    SERIAL_DEBUG("Framebuffer initialized successfully\n");

    // ADD DEBUG PRINTS HERE - IMMEDIATELY AFTER framebuffer_init() returns:
    terminal_writestring("hal_init_devices: Framebuffer parameters after framebuffer_init() returns:\n"); // ADDED DEBUG
    char buf[100];
    sprintf(buf, "  Width: %d, Height: %d, Bits per pixel: %d\n", // ADDED DEBUG
            fb_data.width, fb_data.height, fb_data.bits_per_pixel); // ADDED DEBUG
    terminal_writestring(buf); // ADDED DEBUG
    
    // ... (Rest of hal_init_devices - device inits commented out) ...

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