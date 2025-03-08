# Hardware Abstraction Layer (HAL) - Technical Documentation

## Overview

The Hardware Abstraction Layer (HAL) in Hextrix OS provides a uniform interface between the operating system and hardware devices. The HAL isolates hardware-specific code from the rest of the OS, enabling hardware independence and easier porting to different platforms.

## Design Principles

1. **Abstraction**: Hide hardware complexity behind simple interfaces
2. **Device Independence**: Access hardware through generic device types
3. **Mode Flexibility**: Support both polling and (future) interrupt modes
4. **Extensibility**: Easy addition of new device drivers
5. **Backward Compatibility**: Maintain compatibility with existing OS code

## Core Components

### HAL Core (hal.c, hal.h)

The HAL core provides:
- Device registration and management
- Device type definitions
- Common API for device operations
- HAL initialization

### Device Types

Currently implemented device types:
- `HAL_DEVICE_TIMER`: System timing services
- `HAL_DEVICE_KEYBOARD`: Keyboard input handling

Future planned device types:
- `HAL_DEVICE_DISPLAY`: Screen output
- `HAL_DEVICE_STORAGE`: Storage access

### Operation Modes

- `HAL_MODE_POLLING`: Hardware state checked at regular intervals
- `HAL_MODE_INTERRUPT`: Hardware signals the OS when events occur (planned for future)

## Device Structure

Each device in the HAL is represented by the following structure:

```c
typedef struct {
    uint32_t type;         // Device type
    uint32_t mode;         // Operation mode
    void* private_data;    // Device-specific data
    
    // Common methods
    int (*init)(void* device);
    int (*close)(void* device);
    int (*read)(void* device, void* buffer, uint32_t size);
    int (*write)(void* device, const void* buffer, uint32_t size);
    int (*ioctl)(void* device, uint32_t request, void* arg);
} hal_device_t;

Timer Device (hal_timer.c)
The timer device provides:

System time tracking via tick counter
Sleep and delay functions
Callback registration for regular events

API Functions:

hal_timer_get_ticks(): Get current system tick count
hal_timer_sleep(): Pause execution for specified milliseconds
hal_timer_register_callback(): Register function to be called on timer ticks
hal_timer_poll(): Poll the timer hardware for updates

Keyboard Device (hal_keyboard.c)
The keyboard device provides:

Scancode retrieval from keyboard hardware
Key buffer management
Key availability checking

API Functions:

hal_keyboard_read(): Get next available scancode
hal_keyboard_is_key_available(): Check if keys are in buffer
hal_keyboard_poll(): Poll the keyboard hardware for input

Integration with OS
The HAL is integrated with:

Kernel: Main system initialization
Shell: User input handling
Scheduler: Timer-based task switching

Usage Example

// Initialize HAL
hal_init();
hal_init_devices();

// Register timer callback
hal_timer_register_callback(scheduler_timer_tick);

// Main loop using HAL
while (1) {
    // Update timer
    hal_timer_poll();
    
    // Check for keyboard input
    hal_keyboard_poll();
    
    if (hal_keyboard_is_key_available()) {
        int scancode = hal_keyboard_read();
        process_key(scancode);
    }
}

Future Expansion
The HAL is designed to be easily expanded with:

Additional device drivers
Interrupt-based operation mode
Device discovery mechanisms
Power management functions
Support for additional hardware platforms
