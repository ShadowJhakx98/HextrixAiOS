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

// Add these declarations to the HAL display device functions section in hal.h

// Current declarations (already in the file)
void hal_display_putchar(char c);
void hal_display_writestring(const char* str);
void hal_display_clear(void);

// HAL storage functions
int hal_storage_read_sector(uint32_t sector, void* buffer);
int hal_storage_write_sector(uint32_t sector, const void* buffer);

// New declarations to add
// Basic display control
void hal_display_set_cursor(uint16_t x, uint16_t y);
void hal_display_set_color(uint8_t fg, uint8_t bg);
void hal_display_get_dimensions(uint16_t* width, uint16_t* height);

// Enhanced console features
void hal_display_draw_box(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void hal_display_message_box(const char* title, const char* message);
void hal_display_progress_bar(uint16_t x, uint16_t y, uint16_t width, 
                             uint8_t percent, const char* label);
void hal_display_status_bar(const char* left_text, const char* center_text, 
                           const char* right_text);
#endif // HAL_H