// include/hal_framebuffer.h
#ifndef HAL_FRAMEBUFFER_H
#define HAL_FRAMEBUFFER_H

#include <stdint.h>

// IOCTL commands for framebuffer
#define FB_IOCTL_GET_INFO           0
#define FB_IOCTL_SET_RESOLUTION     1
#define FB_IOCTL_SET_PIXEL          2
#define FB_IOCTL_GET_PIXEL          3
#define FB_IOCTL_CLEAR              4
#define FB_IOCTL_SWAP_BUFFERS       5
#define FB_IOCTL_SET_DOUBLE_BUFFERING 6

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

// Structure for framebuffer information
typedef struct fb_info {
    uint32_t width;           // Framebuffer width in pixels
    uint32_t height;          // Framebuffer height in pixels
    uint32_t bits_per_pixel;  // Bits per pixel
    uint32_t pitch;           // Bytes per row
    void*    buffer;          // Pointer to the framebuffer memory
    uint8_t  double_buffered; // Flag indicating if double buffering is enabled
} fb_info_t;

// Resolution structure
typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t bits_per_pixel;
} fb_resolution_t;

// Pixel structure
typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t color;
} fb_pixel_t;

// Initialize framebuffer
int hal_framebuffer_init(void);

// Drawing functions
void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void fb_draw_circle(uint32_t x0, uint32_t y0, uint32_t radius, uint32_t color);
void fb_fill_circle(uint32_t x0, uint32_t y0, uint32_t radius, uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t color);
void fb_draw_text(uint32_t x, uint32_t y, const char* text, uint32_t color);

// Buffer management
void fb_clear(uint32_t color);
void fb_swap_buffers(void);
void fb_set_double_buffering(uint8_t enable);

// Framebuffer management
void fb_get_info(fb_info_t* info);
void fb_set_resolution(uint32_t width, uint32_t height, uint8_t bits_per_pixel);

#endif // HAL_FRAMEBUFFER_H