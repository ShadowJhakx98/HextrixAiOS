// include/hal.h
#ifndef HAL_H
#define HAL_H

#include <stdint.h>

// HAL initialization
void hal_init(void);
int hal_init_devices(void);

// HAL device types
#define HAL_DEVICE_TIMER    1
#define HAL_DEVICE_KEYBOARD 2
#define HAL_DEVICE_DISPLAY  3
#define HAL_DEVICE_STORAGE  4

// HAL operation modes
#define HAL_MODE_POLLING    0
#define HAL_MODE_INTERRUPT  1

// Generic HAL device structure
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

// HAL timer device functions
uint32_t hal_timer_get_ticks(void);
void hal_timer_sleep(uint32_t ms);
void hal_timer_register_callback(void (*callback)(void));
void hal_timer_poll(void);

// HAL keyboard device functions
int hal_keyboard_read(void);
int hal_keyboard_is_key_available(void);
void hal_keyboard_poll(void);

// HAL display device functions
void hal_display_putchar(char c);
void hal_display_writestring(const char* str);
void hal_display_clear(void);

// HAL storage device functions
int hal_storage_read(uint32_t offset, void* buffer, uint32_t size);
int hal_storage_write(uint32_t offset, const void* buffer, uint32_t size);

// HAL device registration
int hal_register_device(hal_device_t* device);
hal_device_t* hal_get_device(uint32_t type);

#endif // HAL_H