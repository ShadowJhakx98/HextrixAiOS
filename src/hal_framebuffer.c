#include "hal_framebuffer.h"
#include "hal.h"  // Assumes hal_device_t and fb_ioctl are defined
#include <stdlib.h>
#include "kmalloc.h"
#include "string.h"

// Global framebuffer data
fb_data_t fb_data = {0};
static hal_device_t fb_device = {0};

// Simple font data (8x8 bitmap font placeholder)
static const uint8_t font_8x8[128][8] = {
    // Minimal ASCII characters (expand as needed)
    [32] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    [65] = {0x7C, 0x82, 0x82, 0x82, 0xFE, 0x82, 0x82, 0x00}, // 'A'
    // Add more characters as required
};

// Initialization function
static int fb_init(void* device) {
    fb_data.width = 320;  // Simplified resolution
    fb_data.height = 200;
    fb_data.bits_per_pixel = 32;
    fb_data.bytes_per_pixel = 4;
    fb_data.pitch = fb_data.width * fb_data.bytes_per_pixel;
    fb_data.framebuffer_size = fb_data.pitch * fb_data.height;

    fb_data.framebuffer = kmalloc(fb_data.framebuffer_size);
    if (!fb_data.framebuffer) return -1;
    memset(fb_data.framebuffer, 0, fb_data.framebuffer_size);

    fb_data.double_buffering = 0;
    fb_data.back_buffer = NULL;
    fb_data.current_color = FB_COLOR_WHITE;

    return 0;
}

// IOCTL implementation
static int fb_ioctl(void* device, uint32_t request, void* arg) {
    switch (request) {
        case FB_IOCTL_CLEAR: {
            uint32_t color = arg ? *(uint32_t*)arg : 0;
            uint8_t* buffer = fb_data.double_buffering ? fb_data.back_buffer : fb_data.framebuffer;
            uint32_t* pixel = (uint32_t*)buffer;
            for (uint32_t i = 0; i < fb_data.framebuffer_size / 4; i++) {
                pixel[i] = color;
            }
            return 0;
        }
        case FB_IOCTL_ENABLE_DOUBLE_BUFFERING: {
            if (!fb_data.double_buffering) {
                fb_data.back_buffer = kmalloc(fb_data.framebuffer_size);
                if (!fb_data.back_buffer) return -1;
                memcpy(fb_data.back_buffer, fb_data.framebuffer, fb_data.framebuffer_size);
                fb_data.double_buffering = 1;
            }
            return 0;
        }
        case FB_IOCTL_DISABLE_DOUBLE_BUFFERING: {
            if (fb_data.double_buffering) {
                kfree(fb_data.back_buffer);
                fb_data.back_buffer = NULL;
                fb_data.double_buffering = 0;
            }
            return 0;
        }
        case FB_IOCTL_SWAP_BUFFERS: {
            if (fb_data.double_buffering && fb_data.back_buffer) {
                memcpy(fb_data.framebuffer, fb_data.back_buffer, fb_data.framebuffer_size);
            }
            return 0;
        }
        default:
            return -1;
    }
}

// Pixel plotting function
static void fb_plot_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_data.width || y >= fb_data.height) return;
    uint32_t offset = y * fb_data.pitch + x * fb_data.bytes_per_pixel;
    uint8_t* buffer = fb_data.double_buffering ? fb_data.back_buffer : fb_data.framebuffer;
    uint32_t* pixel = (uint32_t*)(buffer + offset);
    *pixel = color;
}

// Public API functions
void fb_clear_screen(uint32_t color) {
    fb_ioctl(&fb_device, FB_IOCTL_CLEAR, &color);
    if (fb_data.double_buffering) {
        fb_swap_buffers();
    }
}

void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    fb_plot_pixel(x, y, color);
}

void fb_swap_buffers(void) {
    fb_ioctl(&fb_device, FB_IOCTL_SWAP_BUFFERS, NULL);
}

hal_device_t* framebuffer_init(void) {
    fb_device.init = fb_init;
    fb_device.ioctl = fb_ioctl;
    if (fb_init(&fb_device) != 0) return NULL;
    return &fb_device;
}

int fb_get_info(fb_info_t* info) {
    if (!info) return -1;
    info->width = fb_data.width;
    info->height = fb_data.height;
    info->bits_per_pixel = fb_data.bits_per_pixel;
    info->pitch = fb_data.pitch;
    info->buffer = fb_data.framebuffer;
    info->double_buffered = fb_data.double_buffering;
    return 0;
}

void fb_set_double_buffering(uint8_t enabled) {
    if (enabled) {
        fb_ioctl(&fb_device, FB_IOCTL_ENABLE_DOUBLE_BUFFERING, NULL);
    } else {
        fb_ioctl(&fb_device, FB_IOCTL_DISABLE_DOUBLE_BUFFERING, NULL);
    }
}

void fb_draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color, int filled) {
    if (filled) {
        for (uint32_t i = y; i < y + height && i < fb_data.height; i++) {
            for (uint32_t j = x; j < x + width && j < fb_data.width; j++) {
                fb_plot_pixel(j, i, color);
            }
        }
    } else {
        for (uint32_t i = x; i < x + width && i < fb_data.width; i++) {
            fb_plot_pixel(i, y, color);
            fb_plot_pixel(i, y + height - 1, color);
        }
        for (uint32_t i = y; i < y + height && i < fb_data.height; i++) {
            fb_plot_pixel(x, i, color);
            fb_plot_pixel(x + width - 1, i, color);
        }
    }
}

void fb_draw_text(uint32_t x, uint32_t y, const char* text, uint32_t color) {
    if (!text) return;
    uint32_t pos_x = x;
    for (int i = 0; text[i]; i++) {
        uint8_t c = text[i];
        if (c < 32 || c > 127) continue; // Basic ASCII only
        const uint8_t* glyph = font_8x8[c];
        for (int gy = 0; gy < 8; gy++) {
            for (int gx = 0; gx < 8; gx++) {
                if (glyph[gy] & (1 << (7 - gx))) {
                    fb_plot_pixel(pos_x + gx, y + gy, color);
                }
            }
        }
        pos_x += 8;
    }
}

void fb_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t color) {
    int dx = abs((int)x1 - (int)x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs((int)y1 - (int)y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (1) {
        fb_plot_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void fb_draw_circle(uint32_t center_x, uint32_t center_y, uint32_t radius, uint32_t color, int filled) {
    int x = radius;
    int y = 0;
    int err = 0;

    while (x >= y) {
        if (filled) {
            fb_draw_line(center_x - x, center_y + y, center_x + x, center_y + y, color);
            fb_draw_line(center_x - y, center_y + x, center_x + y, center_y + x, color);
            fb_draw_line(center_x - x, center_y - y, center_x + x, center_y - y, color);
            fb_draw_line(center_x - y, center_y - x, center_x + y, center_y - x, color);
        } else {
            fb_plot_pixel(center_x + x, center_y + y, color);
            fb_plot_pixel(center_x - x, center_y + y, color);
            fb_plot_pixel(center_x + x, center_y - y, color);
            fb_plot_pixel(center_x - x, center_y - y, color);
            fb_plot_pixel(center_x + y, center_y + x, color);
            fb_plot_pixel(center_x - y, center_y + x, color);
            fb_plot_pixel(center_x + y, center_y - x, color);
            fb_plot_pixel(center_x - y, center_y - x, color);
        }

        y += 1;
        err += 1 + 2 * y;
        if (2 * (err - x) + 1 > 0) {
            x -= 1;
            err += 1 - 2 * x;
        }
    }
}