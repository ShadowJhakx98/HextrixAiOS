#ifndef HAL_FRAMEBUFFER_H
#define HAL_FRAMEBUFFER_H

#include <stdint.h>

// IOCTL commands
#define FB_IOCTL_CLEAR                  1
#define FB_IOCTL_ENABLE_DOUBLE_BUFFERING 2
#define FB_IOCTL_DISABLE_DOUBLE_BUFFERING 3
#define FB_IOCTL_SWAP_BUFFERS           4

// Color constants (ARGB format)
#define FB_COLOR_BLACK       0xFF000000
#define FB_COLOR_WHITE       0xFFFFFFFF
#define FB_COLOR_RED         0xFFFF0000
#define FB_COLOR_GREEN       0xFF00FF00
#define FB_COLOR_BLUE        0xFF0000FF
#define FB_COLOR_YELLOW      0xFFFFFF00
#define FB_COLOR_CYAN        0xFF00FFFF
#define FB_COLOR_MAGENTA     0xFFFF00FF
#define FB_COLOR_GRAY        0xFF808080
#define FB_COLOR_DARK_GRAY   0xFF404040
#define FB_COLOR_LIGHT_GRAY  0xFFC0C0C0
#define FB_COLOR_ORANGE      0xFFFF8000
#define FB_COLOR_PURPLE      0xFF800080
#define FB_COLOR_BROWN       0xFF8B4513
#define FB_COLOR_TRANSPARENT 0x00000000

// Framebuffer info structure
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bits_per_pixel;
    uint32_t pitch;
    void*    buffer;
    uint8_t  double_buffered;
} fb_info_t;

// Framebuffer data structure
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bits_per_pixel;
    uint8_t bytes_per_pixel;
    uint32_t framebuffer_addr;
    uint8_t* framebuffer;  // <-- IMPORTANT: Make sure it's named 'framebuffer' and is a pointer
    uint32_t framebuffer_size;
    uint32_t current_color;
    uint8_t double_buffering;
    uint8_t* back_buffer;
} fb_data_t;

extern fb_data_t fb_data;

// Function declarations
void fb_clear_screen(uint32_t color);
void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_swap_buffers(void);
struct hal_device_t* framebuffer_init(void);
int fb_get_info(fb_info_t* info);
void fb_set_double_buffering(uint8_t enabled);
void fb_draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color, int filled);
void fb_draw_text(uint32_t x, uint32_t y, const char* text, uint32_t color);
void fb_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color);
void fb_draw_circle(uint32_t center_x, uint32_t center_y, uint32_t radius, uint32_t color, int filled);

#endif