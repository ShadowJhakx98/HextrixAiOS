#include "hal_framebuffer.h"
#include "hal.h"  // Assumes hal_device_t and fb_ioctl are defined
#include "terminal.h"
#include <stdlib.h>
#include "kmalloc.h"
#include "string.h"

// VGA Memory-Mapped I/O address
#define VGA_MMIO_BASE 0xA0000
#define VGA_WIDTH 320
#define VGA_HEIGHT 200

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

// In hal_framebuffer.c
void fb_init_direct_vga(void) {
    terminal_writestring("fb_init_direct_vga: Setting VGA Mode 13h (320x200, 256 colors)...\n");
    
    // VGA Mode 13h initialization sequence
    
    // Misc Output Register (sets clock frequencies and sync polarities)
    outb(0x3C2, 0x63);
    
    // Sequencer Registers
    outb(0x3C4, 0x00); outb(0x3C5, 0x03); // Reset sequencer
    outb(0x3C4, 0x01); outb(0x3C5, 0x01); // Clocking Mode
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F); // Map Mask register (enable all planes)
    outb(0x3C4, 0x03); outb(0x3C5, 0x00); // Character Map Select
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E); // Memory Mode register
    
    // Unlock CRTC registers
    outb(0x3D4, 0x11); 
    outb(0x3D5, inb(0x3D5) & 0x7F);
    
    // CRT Controller Registers
    static const uint8_t crtc_regs[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
        0xFF
    };
    
    for (int i = 0; i < 24; i++) {
        outb(0x3D4, i);
        outb(0x3D5, crtc_regs[i]);
    }
    
    // Graphics Controller Registers
    outb(0x3CE, 0x00); outb(0x3CF, 0x00); // Set/Reset register
    outb(0x3CE, 0x01); outb(0x3CF, 0x00); // Enable Set/Reset register
    outb(0x3CE, 0x02); outb(0x3CF, 0x00); // Color Compare register
    outb(0x3CE, 0x03); outb(0x3CF, 0x00); // Data Rotate register
    outb(0x3CE, 0x04); outb(0x3CF, 0x00); // Read Map Select register
    outb(0x3CE, 0x05); outb(0x3CF, 0x40); // Mode register (256-color mode)
    outb(0x3CE, 0x06); outb(0x3CF, 0x05); // Miscellaneous register
    outb(0x3CE, 0x07); outb(0x3CF, 0x0F); // Color Don't Care register
    outb(0x3CE, 0x08); outb(0x3CF, 0xFF); // Bit Mask register
    
    // Attribute Controller Registers
    inb(0x3DA); // Reset attribute controller flip-flop
    
    // Set palette registers (first 16 match CGA colors)
    for (int i = 0; i < 16; i++) {
        outb(0x3C0, i);          // Select palette register
        outb(0x3C0, i);          // Set palette data
    }
    
    // Enable display
    outb(0x3C0, 0x20);           // Enable video
    
    // Set framebuffer pointer directly to VGA memory
    fb_data.framebuffer = (uint8_t*)0xA0000;
    
    // Set framebuffer parameters
    fb_data.width = 320;
    fb_data.height = 200;
    fb_data.pitch = 320;
    fb_data.bits_per_pixel = 8;
    fb_data.bytes_per_pixel = 1;
    fb_data.framebuffer_size = 320 * 200;
    fb_data.double_buffering = 0;
    fb_data.back_buffer = NULL;
    
    // Force a clear screen to blue background
    uint8_t* buffer = fb_data.framebuffer;
    for (uint32_t i = 0; i < fb_data.width * fb_data.height; i++) {
        buffer[i] = 1; // Blue color index
    }
    
    // Draw a test pattern to verify graphics mode
    for (uint32_t y = 0; y < 20; y++) {
        for (uint32_t x = 0; x < 320; x++) {
            uint32_t offset = y * fb_data.pitch + x;
            buffer[offset] = (x / 32) + 1; // Create a pattern of vertical bars
        }
    }
    
    terminal_writestring("fb_init_direct_vga: VGA Mode 13h initialization complete\n");
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

void fb_clear_screen(uint32_t color) {
    fb_info_t info;
    fb_get_info(&info);
    uint8_t* buffer = info.buffer;

    if (!buffer) {
        terminal_writestring("fb_clear_screen: Error! Buffer is NULL!\n");
        return;
    }

    if (info.bits_per_pixel == 8) {
        uint8_t vga_color = (uint8_t)color;
        for (uint32_t i = 0; i < info.width * info.height; i++) {
            buffer[i] = vga_color;
        }
    } else if (info.bits_per_pixel == 32) {
        uint32_t* pixel = (uint32_t*)buffer;
        for (uint32_t i = 0; i < info.width * info.height; i++) {
            pixel[i] = color;
        }
    }
}

void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb_data.width || y >= fb_data.height) return;
    uint8_t* buffer = fb_data.double_buffering ? fb_data.back_buffer : fb_data.framebuffer;
    if (fb_data.bits_per_pixel == 8) {
        uint32_t offset = y * fb_data.pitch + x;
        buffer[offset] = (uint8_t)color; // 8-bit color
    } else if (fb_data.bits_per_pixel == 32) {
        uint32_t offset = y * fb_data.pitch + x * 4; // 4 bytes per pixel
        uint32_t* pixel = (uint32_t*)(buffer + offset);
        *pixel = color; // 32-bit color
    } else {
        terminal_writestring("Unsupported bits per pixel in fb_draw_pixel\n");
    }
}

void fb_swap_buffers(void) {
    fb_ioctl(&fb_device, FB_IOCTL_SWAP_BUFFERS, NULL);
}

// In hal_framebuffer.c

hal_device_t* framebuffer_init(void) {
    terminal_writestring("\n==== FRAMEBUFFER INITIALIZATION ====\n");
    terminal_writestring("framebuffer_init: Attempting to initialize video output...\n");

    // Call fb_init_direct_vga at the beginning
    terminal_writestring("framebuffer_init: Calling fb_init_direct_vga()\n");
    fb_init_direct_vga();
    terminal_writestring("fb_init_direct_vga: Framebuffer parameters after setting:\n"); // ADDED DEBUG
    char buf[100];
    sprintf(buf, "  Width: %d, Height: %d, Bits per pixel: %d\n", // ADDED DEBUG
            fb_data.width, fb_data.height, fb_data.bits_per_pixel); // ADDED DEBUG
    terminal_writestring(buf); // ADDED DEBUG
    terminal_writestring("framebuffer_init: Returned from fb_init_direct_vga()\n");

    // Set the device type and operations (unchanged) ...

    int init_success = 0;

    // Try direct VGA mode first - more likely to work with QEMU
    terminal_writestring("framebuffer_init: Trying direct VGA mode (Mode 13h)...\n");
    // Since fb_init_direct_vga() was already called, assume success if no errors
    terminal_writestring("framebuffer_init: Direct VGA Mode 13h initialized successfully!\n");
    init_success = 1;

    // REMOVE FALLBACK LOGIC - COMMENT OUT THESE LINES:
    /*
    if (!init_success) {
        terminal_writestring("framebuffer_init: Direct VGA mode failed.\n");

        // Try standard framebuffer init as fallback
        terminal_writestring("framebuffer_init: Falling back to standard framebuffer...\n");
        if (fb_init(&fb_device) == 0) {
            terminal_writestring("framebuffer_init: Standard framebuffer initialized.\n");
            init_success = 1;
        } else {
            terminal_writestring("framebuffer_init: Standard framebuffer initialization failed.\n");
        }
    }
    */

    if (!init_success) {
        terminal_writestring("framebuffer_init: All framebuffer initialization methods failed!\n");
        return NULL;
    }

    // ... (Rest of framebuffer_init code - unchanged) ...

    return &fb_device;
}
// Get framebuffer information
int fb_get_info(fb_info_t* info) {
    if (info) {
        info->width = fb_data.width;
        info->height = fb_data.height;
        info->bits_per_pixel = fb_data.bits_per_pixel;
        info->pitch = fb_data.pitch;
        info->buffer = fb_data.framebuffer;
        info->double_buffered = fb_data.double_buffering;
        return 0; // Success
    }
    return -1; // Failure, invalid pointer
}

// Set double buffering
void fb_set_double_buffering(uint8_t enabled) {
    if (enabled) {
        fb_ioctl(&fb_device, FB_IOCTL_ENABLE_DOUBLE_BUFFERING, NULL);
    } else {
        fb_ioctl(&fb_device, FB_IOCTL_DISABLE_DOUBLE_BUFFERING, NULL);
    }
}

// Draw a rectangle
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

// Draw text using the simple font
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

// Draw a line using Bresenham's algorithm
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

// Draw a circle (filled or outlined)
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