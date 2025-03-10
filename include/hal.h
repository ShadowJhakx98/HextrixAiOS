#ifndef HAL_H
#define HAL_H

void outb(unsigned short port, unsigned char value);
unsigned char inb(unsigned short port);
int hal_framebuffer_is_ready(void);
int hal_is_system_stable(void);


#include <stdint.h>
#include "hal_framebuffer.h"  // Include this first to get fb_info_t definition
#include "hal_mouse.h"

// HAL initialization
int hal_init(void);
int hal_init_devices(void);

// HAL device types
#define HAL_DEVICE_TIMER        1
#define HAL_DEVICE_KEYBOARD     2
#define HAL_DEVICE_DISPLAY      3
#define HAL_DEVICE_STORAGE      4
#define HAL_DEVICE_ATA          5
#define HAL_DEVICE_FRAMEBUFFER  6  // Add this line
#define HAL_DEVICE_MOUSE        7  // Add this line too for GUI support

// HAL operation modes
#define HAL_MODE_POLLING    0
#define HAL_MODE_INTERRUPT  1

typedef struct hal_device_t {
    uint32_t type;
    uint32_t mode;
    void* private_data;
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

// Mouse button definitions
#define MOUSE_BUTTON_LEFT    0x01
#define MOUSE_BUTTON_RIGHT   0x02
#define MOUSE_BUTTON_MIDDLE  0x04

// HAL mouse device functions (add these)
void hal_mouse_poll(void);
int hal_mouse_get_position(int16_t* x, int16_t* y);
int hal_mouse_get_buttons(uint8_t* buttons);
int hal_mouse_register_handler(int (*handler)(mouse_event_t* event));

// HAL display device functions
void hal_display_putchar(char c);
void hal_display_writestring(const char* str);
void hal_display_clear(void);
void hal_display_set_cursor(uint16_t x, uint16_t y);
void hal_display_set_color(uint8_t fg, uint8_t bg);
void hal_display_get_dimensions(uint16_t* width, uint16_t* height);
void hal_display_draw_box(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void hal_display_message_box(const char* title, const char* message);
void hal_display_progress_bar(uint16_t x, uint16_t y, uint16_t width, 
                             uint8_t percent, const char* label);
void hal_display_status_bar(const char* left_text, const char* center_text, 
                           const char* right_text);

// HAL framebuffer device functions
int hal_framebuffer_init(void);
void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_draw_circle(uint32_t center_x, uint32_t center_y, uint32_t radius, uint32_t color, int filled);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color); // Already present
void fb_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color); // Already present
void fb_fill_circle(uint32_t x0, uint32_t y0, uint32_t radius, uint32_t color);
void fb_draw_text(uint32_t x, uint32_t y, const char* text, uint32_t color);
void fb_clear(uint32_t color);
void fb_swap_buffers(void);
void fb_set_double_buffering(uint8_t enable);
int fb_get_info(fb_info_t* info);
void fb_set_resolution(uint32_t width, uint32_t height, uint8_t bits_per_pixel);

// HAL storage device functions
int hal_storage_read(uint32_t offset, void* buffer, uint32_t size);
int hal_storage_write(uint32_t offset, const void* buffer, uint32_t size);
int hal_storage_read_sector(uint32_t sector, void* buffer);
int hal_storage_write_sector(uint32_t sector, const void* buffer);

// HAL ATA device functions
void hal_ata_init(void);
int hal_ata_detect_devices(void);
int hal_ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, void* buffer);
int hal_ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, const void* buffer);
int hal_ata_identify(uint8_t drive, void* buffer);
void hal_ata_print_info(void);

// HAL device registration
int hal_register_device(hal_device_t* device);
hal_device_t* hal_get_device(uint32_t type);

#endif // HAL_H