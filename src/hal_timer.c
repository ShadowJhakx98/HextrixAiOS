// src/hal_timer.c
#include "hal.h"
#include "io.h"
#include "terminal.h"
#include "scheduler.h"

// Use the existing timer_ticks from interrupts.c instead of defining a new one
extern volatile uint32_t timer_ticks;

// Callback function pointer
static void (*timer_callback)(void) = 0;

// Timer device private data
typedef struct {
    uint32_t frequency;
    uint32_t counter;
} timer_data_t;

// Local timer device
static timer_data_t timer_data = {0};
static hal_device_t timer_device = {0};

// Device-specific functions
static int timer_init(void* device) {
    hal_device_t* dev = (hal_device_t*)device;
    timer_data_t* data = (timer_data_t*)dev->private_data;
    
    // Initialize data
    data->frequency = 100; // 100Hz default (10ms)
    data->counter = 0;
    
    terminal_writestring("HAL Timer initialized in polling mode\n");
    
    return 0;
}

static int timer_close(void* device) {
    return 0;
}

static int timer_read(void* device, void* buffer, uint32_t size) {
    if (size < sizeof(uint32_t)) {
        return -1;
    }
    
    // Read current ticks
    *((uint32_t*)buffer) = timer_ticks;
    
    return sizeof(uint32_t);
}

static int timer_write(void* device, const void* buffer, uint32_t size) {
    return -1; // Timer is read-only
}

static int timer_ioctl(void* device, uint32_t request, void* arg) {
    hal_device_t* dev = (hal_device_t*)device;
    timer_data_t* data = (timer_data_t*)dev->private_data;
    
    switch (request) {
        case 0: // Set frequency
            data->frequency = *(uint32_t*)arg;
            return 0;
            
        case 1: // Register callback
            timer_callback = (void (*)(void))arg;
            return 0;
            
        default:
            return -1;
    }
}

// Poll timer based on PIT counter
void hal_timer_poll(void) {
    // Read timer status - we can check port 0x40 to poll the timer
    static uint32_t last_time = 0;
    uint32_t current_time = 0;
    
    // Read the timer counter from PIT channel 0
    outb(0x43, 0x00);  // Command: channel 0, read counter
    current_time = inb(0x40);   // Read low byte
    current_time |= inb(0x40) << 8;  // Read high byte
    
    // Timer counts down, so we can detect a change
    if (current_time != last_time) {
        last_time = current_time;
        timer_ticks++;
        
        // Call callback if registered
        if (timer_callback) {
            timer_callback();
        }
    }
}

// HAL timer interface functions
uint32_t hal_timer_get_ticks(void) {
    return timer_ticks;
}

void hal_timer_sleep(uint32_t ms) {
    uint32_t start_ticks = timer_ticks;
    uint32_t target_ticks = start_ticks + (ms / 10) + 1;
    
    while (timer_ticks < target_ticks) {
        hal_timer_poll();
    }
}

void hal_timer_register_callback(void (*callback)(void)) {
    timer_callback = callback;
}

// Initialize and register timer device
int hal_timer_init(void) {
    // Setup device
    timer_device.type = HAL_DEVICE_TIMER;
    timer_device.mode = HAL_MODE_POLLING;
    timer_device.private_data = &timer_data;
    
    // Set functions
    timer_device.init = timer_init;
    timer_device.close = timer_close;
    timer_device.read = timer_read;
    timer_device.write = timer_write;
    timer_device.ioctl = timer_ioctl;
    
    // Register with HAL
    return hal_register_device(&timer_device);
}