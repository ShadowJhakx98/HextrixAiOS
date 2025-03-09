// src/hal_framebuffer.c
#include "hal_framebuffer.h"
#include "hal.h"  // Added this include for hal_device_t definition
#include "io.h"
#include "terminal.h"
#include "stdio.h"
#include "kmalloc.h"
#include "string.h"
#include <stdlib.h> // Added for abs() function

// VBE (VESA BIOS Extensions) mode information structure
typedef struct {
    uint16_t mode_attributes;
    uint8_t window_a_attributes;
    uint8_t window_b_attributes;
    uint16_t window_granularity;
    uint16_t window_size;
    uint16_t window_a_segment;
    uint16_t window_b_segment;
    uint32_t window_function_ptr;
    uint16_t bytes_per_scanline;
    
    // VBE 1.2+
    uint16_t x_resolution;
    uint16_t y_resolution;
    uint8_t x_char_size;
    uint8_t y_char_size;
    uint8_t num_planes;
    uint8_t bits_per_pixel;
    uint8_t num_banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t num_image_pages;
    uint8_t reserved1;
    
    // Direct Color fields
    uint8_t red_mask_size;
    uint8_t red_field_position;
    uint8_t green_mask_size;
    uint8_t green_field_position;
    uint8_t blue_mask_size;
    uint8_t blue_field_position;
    uint8_t reserved_mask_size;
    uint8_t reserved_field_position;
    uint8_t direct_color_mode_info;
    
    // VBE 2.0+
    uint32_t physical_base_ptr;  // Framebuffer address
    uint32_t reserved2;
    uint16_t reserved3;
    
    // VBE 3.0+
    uint16_t linear_bytes_per_scanline;
    uint8_t bank_number_of_image_pages;
    uint8_t linear_number_of_image_pages;
    uint8_t linear_red_mask_size;
    uint8_t linear_red_field_position;
    uint8_t linear_green_mask_size;
    uint8_t linear_green_field_position;
    uint8_t linear_blue_mask_size;
    uint8_t linear_blue_field_position;
    uint8_t linear_reserved_mask_size;
    uint8_t linear_reserved_field_position;
    uint32_t max_pixel_clock;
    
    uint8_t reserved4[190];  // Reserved space to make 256 bytes total
} __attribute__((packed)) vbe_mode_info_t;

// Framebuffer device private data
typedef struct {
    uint32_t width;               // Width in pixels
    uint32_t height;              // Height in pixels
    uint32_t pitch;               // Bytes per scanline
    uint8_t bits_per_pixel;       // Bits per pixel
    uint8_t bytes_per_pixel;      // Bytes per pixel
    uint32_t framebuffer_addr;    // Physical address of framebuffer
    uint8_t* framebuffer;         // Virtual address of framebuffer
    uint32_t framebuffer_size;    // Size of framebuffer in bytes
    uint32_t current_color;       // Current drawing color (ARGB)
    uint8_t double_buffering;     // Is double buffering enabled?
    uint8_t* back_buffer;         // Back buffer for double buffering
} fb_data_t;

// Local framebuffer device
static fb_data_t fb_data = {0};
static hal_device_t fb_device = {0};

// Color conversion functions
static inline uint32_t color_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

static inline uint8_t color_get_r(uint32_t color) {
    return (color >> 16) & 0xFF;
}

static inline uint8_t color_get_g(uint32_t color) {
    return (color >> 8) & 0xFF;
}

static inline uint8_t color_get_b(uint32_t color) {
    return color & 0xFF;
}

// Plot a pixel directly to the framebuffer
static void fb_plot_pixel(uint32_t x, uint32_t y, uint32_t color) {
    fb_data_t* data = &fb_data;
    
    // Check boundaries
    if (x >= data->width || y >= data->height)
        return;
    
    // Calculate offset in framebuffer
    uint32_t offset = y * data->pitch + x * data->bytes_per_pixel;
    
    // Handle different bit depths
    if (data->bits_per_pixel == 32) {
        // 32 bits per pixel
        uint32_t* pixel = (uint32_t*)(data->framebuffer + offset);
        *pixel = color;
    } else if (data->bits_per_pixel == 24) {
        // 24 bits per pixel
        uint8_t* pixel = data->framebuffer + offset;
        pixel[0] = color_get_b(color);
        pixel[1] = color_get_g(color);
        pixel[2] = color_get_r(color);
    } else if (data->bits_per_pixel == 16) {
        // 16 bits per pixel (typically 5:6:5 RGB)
        uint16_t r = color_get_r(color) >> 3;
        uint16_t g = color_get_g(color) >> 2;
        uint16_t b = color_get_b(color) >> 3;
        uint16_t rgb = (r << 11) | (g << 5) | b;
        
        uint16_t* pixel = (uint16_t*)(data->framebuffer + offset);
        *pixel = rgb;
    } else if (data->bits_per_pixel == 8) {
        // 8 bits per pixel (assuming grayscale)
        uint8_t gray = (color_get_r(color) + color_get_g(color) + color_get_b(color)) / 3;
        data->framebuffer[offset] = gray;
    }
}

// Plot a pixel to the active buffer (back buffer if double buffering is enabled)
static void fb_plot_pixel_buffered(uint32_t x, uint32_t y, uint32_t color) {
    fb_data_t* data = &fb_data;
    
    // Check boundaries
    if (x >= data->width || y >= data->height)
        return;
    
    // Calculate offset in buffer
    uint32_t offset = y * data->pitch + x * data->bytes_per_pixel;
    
    // Use back buffer if double buffering is enabled, otherwise use framebuffer
    uint8_t* buffer = data->double_buffering ? data->back_buffer : data->framebuffer;
    
    // Handle different bit depths
    if (data->bits_per_pixel == 32) {
        // 32 bits per pixel
        uint32_t* pixel = (uint32_t*)(buffer + offset);
        *pixel = color;
    } else if (data->bits_per_pixel == 24) {
        // 24 bits per pixel
        uint8_t* pixel = buffer + offset;
        pixel[0] = color_get_b(color);
        pixel[1] = color_get_g(color);
        pixel[2] = color_get_r(color);
    } else if (data->bits_per_pixel == 16) {
        // 16 bits per pixel (typically 5:6:5 RGB)
        uint16_t r = color_get_r(color) >> 3;
        uint16_t g = color_get_g(color) >> 2;
        uint16_t b = color_get_b(color) >> 3;
        uint16_t rgb = (r << 11) | (g << 5) | b;
        
        uint16_t* pixel = (uint16_t*)(buffer + offset);
        *pixel = rgb;
    } else if (data->bits_per_pixel == 8) {
        // 8 bits per pixel (assuming grayscale)
        uint8_t gray = (color_get_r(color) + color_get_g(color) + color_get_b(color)) / 3;
        buffer[offset] = gray;
    }
}

// Initialize framebuffer using VESA VBE mode
static int fb_init_vesa(uint16_t width, uint16_t height, uint8_t bits_per_pixel) {
    // Note: In a real implementation, this would use BIOS interrupt 0x10 to set the video mode
    // and obtain the VBE mode information. Since we're in protected mode, we'd typically do this
    // during boot and pass the information via multiboot.
    
    // For simulation purposes, we'll just allocate a buffer to represent the framebuffer
    // In a real implementation, this would be mapped to the actual hardware framebuffer
    
    fb_data_t* data = &fb_data;
    
    // Set resolution and color depth
    data->width = width;
    data->height = height;
    data->bits_per_pixel = bits_per_pixel;
    
    // Calculate bytes per pixel and pitch
    data->bytes_per_pixel = (bits_per_pixel + 7) / 8;  // Round up to nearest byte
    data->pitch = width * data->bytes_per_pixel;
    
    // Calculate framebuffer size
    data->framebuffer_size = data->height * data->pitch;
    
    // Allocate memory for framebuffer (simulation only)
    data->framebuffer = kmalloc(data->framebuffer_size);
    if (!data->framebuffer) {
        terminal_writestring("Failed to allocate memory for framebuffer\n");
        return -1;
    }
    
    // Set default color to white
    data->current_color = color_rgb(255, 255, 255);
    
    // Clear the framebuffer
    memset(data->framebuffer, 0, data->framebuffer_size);
    
    // Initialize double buffering if requested
    if (data->double_buffering) {
        data->back_buffer = kmalloc(data->framebuffer_size);
        if (!data->back_buffer) {
            terminal_writestring("Failed to allocate memory for back buffer\n");
            data->double_buffering = 0;
        } else {
            memset(data->back_buffer, 0, data->framebuffer_size);
        }
    }
    
    terminal_printf("Framebuffer initialized: %dx%dx%d\n", width, height, bits_per_pixel);
    
    return 0;
}

// Device-specific functions
static int fb_init(void* device) {
    hal_device_t* dev = (hal_device_t*)device;
    fb_data_t* data = (fb_data_t*)dev->private_data;
    
    // Initialize framebuffer with default resolution (640x480x32)
    if (fb_init_vesa(640, 480, 32) != 0) {
        terminal_writestring("Failed to initialize framebuffer\n");
        return -1;
    }
    
    terminal_writestring("HAL Framebuffer initialized\n");
    
    return 0;
}

static int fb_close(void* device) {
    fb_data_t* data = &fb_data;
    
    // Free allocated memory (simulation only)
    if (data->framebuffer) {
        kfree(data->framebuffer);
        data->framebuffer = NULL;
    }
    
    if (data->back_buffer) {
        kfree(data->back_buffer);
        data->back_buffer = NULL;
    }
    
    return 0;
}

static int fb_read(void* device, void* buffer, uint32_t size) {
    fb_data_t* data = &fb_data;
    
    // Read from framebuffer
    if (size > data->framebuffer_size)
        size = data->framebuffer_size;
    
    memcpy(buffer, data->framebuffer, size);
    
    return size;
}

static int fb_write(void* device, const void* buffer, uint32_t size) {
    fb_data_t* data = &fb_data;
    
    // Write to framebuffer
    if (size > data->framebuffer_size)
        size = data->framebuffer_size;
    
    memcpy(data->framebuffer, buffer, size);
    
    return size;
}

static int fb_ioctl(void* device, uint32_t request, void* arg) {
    fb_data_t* data = &fb_data;
    
    switch (request) {
        case FB_IOCTL_GET_INFO: {
            // Get framebuffer information
            fb_info_t* info = (fb_info_t*)arg;
            if (info) {
                info->width = data->width;
                info->height = data->height;
                info->bits_per_pixel = data->bits_per_pixel;
                info->pitch = data->pitch;
                info->buffer = data->framebuffer;
                info->double_buffered = data->double_buffering;
                return 0;
            }
            break;
        }
        case FB_IOCTL_SET_RESOLUTION: {
            // Set framebuffer resolution
            fb_resolution_t* res = (fb_resolution_t*)arg;
            if (res) {
                // Close current framebuffer
                fb_close(device);
                
                // Initialize with new resolution
                if (fb_init_vesa(res->width, res->height, res->bits_per_pixel) != 0) {
                    // If failed, try to restore previous resolution
                    fb_init_vesa(data->width, data->height, data->bits_per_pixel);
                    return -1;
                }
                return 0;
            }
            break;
        }
        case FB_IOCTL_SET_PIXEL: {
            // Set a single pixel
            fb_pixel_t* pixel = (fb_pixel_t*)arg;
            if (pixel) {
                fb_plot_pixel_buffered(pixel->x, pixel->y, pixel->color);
                return 0;
            }
            break;
        }
        case FB_IOCTL_GET_PIXEL: {
            // Get a single pixel
            fb_pixel_t* pixel = (fb_pixel_t*)arg;
            if (pixel) {
                // Check boundaries
                if (pixel->x >= data->width || pixel->y >= data->height)
                    return -1;
                
                // Calculate offset in framebuffer
                uint32_t offset = pixel->y * data->pitch + pixel->x * data->bytes_per_pixel;
                
                // Use back buffer if double buffering is enabled, otherwise use framebuffer
                uint8_t* buffer = data->double_buffering ? data->back_buffer : data->framebuffer;
                
                // Handle different bit depths
                if (data->bits_per_pixel == 32) {
                    // 32 bits per pixel
                    pixel->color = *(uint32_t*)(buffer + offset);
                } else if (data->bits_per_pixel == 24) {
                    // 24 bits per pixel
                    uint8_t* p = buffer + offset;
                    pixel->color = (p[2] << 16) | (p[1] << 8) | p[0];
                } else if (data->bits_per_pixel == 16) {
                    // 16 bits per pixel (typically 5:6:5 RGB)
                    uint16_t rgb = *(uint16_t*)(buffer + offset);
                    uint8_t r = ((rgb >> 11) & 0x1F) << 3;
                    uint8_t g = ((rgb >> 5) & 0x3F) << 2;
                    uint8_t b = (rgb & 0x1F) << 3;
                    pixel->color = (r << 16) | (g << 8) | b;
                } else if (data->bits_per_pixel == 8) {
                    // 8 bits per pixel (assuming grayscale)
                    uint8_t gray = buffer[offset];
                    pixel->color = (gray << 16) | (gray << 8) | gray;
                }
                
                return 0;
            }
            break;
        }
        case FB_IOCTL_CLEAR: {
            // Clear the framebuffer
            uint32_t color = 0;
            if (arg) {
                color = *(uint32_t*)arg;
            }
            
            // Fill the active buffer with the specified color
            uint8_t* buffer = data->double_buffering ? data->back_buffer : data->framebuffer;
            
            // For most bit depths, we can't use memset as it only sets bytes
            for (uint32_t y = 0; y < data->height; y++) {
                for (uint32_t x = 0; x < data->width; x++) {
                    fb_plot_pixel_buffered(x, y, color);
                }
            }
            
            return 0;
        }
        case FB_IOCTL_SWAP_BUFFERS: {
            // Swap front and back buffers
            if (data->double_buffering && data->back_buffer) {
                // Copy back buffer to framebuffer
                memcpy(data->framebuffer, data->back_buffer, data->framebuffer_size);
                return 0;
            }
            break;
        }
        case FB_IOCTL_SET_DOUBLE_BUFFERING: {
            // Enable or disable double buffering
            uint8_t enable = *(uint8_t*)arg;
            
            if (enable && !data->double_buffering) {
                // Enable double buffering
                data->back_buffer = kmalloc(data->framebuffer_size);
                if (!data->back_buffer) {
                    return -1;
                }
                memset(data->back_buffer, 0, data->framebuffer_size);
                data->double_buffering = 1;
            } else if (!enable && data->double_buffering) {
                // Disable double buffering
                kfree(data->back_buffer);
                data->back_buffer = NULL;
                data->double_buffering = 0;
            }
            
            return 0;
        }
    }
    
    return -1;  // Invalid request or argument
}

// HAL framebuffer interface functions

void fb_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    fb_pixel_t pixel;
    pixel.x = x;
    pixel.y = y;
    pixel.color = color;
    
    fb_ioctl(&fb_device, FB_IOCTL_SET_PIXEL, &pixel);
}

void fb_draw_line(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    // Bresenham's line algorithm
    int dx = abs((int)x2 - (int)x1);
    int dy = -abs((int)y2 - (int)y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    int e2;
    
    while (1) {
        fb_draw_pixel(x1, y1, color);
        
        if (x1 == x2 && y1 == y2)
            break;
        
        e2 = 2 * err;
        if (e2 >= dy) {
            if (x1 == x2)
                break;
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            if (y1 == y2)
                break;
            err += dx;
            y1 += sy;
        }
    }
}

void fb_draw_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    // Draw horizontal lines
    for (uint32_t i = 0; i < width; i++) {
        fb_draw_pixel(x + i, y, color);
        fb_draw_pixel(x + i, y + height - 1, color);
    }
    
    // Draw vertical lines
    for (uint32_t i = 1; i < height - 1; i++) {
        fb_draw_pixel(x, y + i, color);
        fb_draw_pixel(x + width - 1, y + i, color);
    }
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    for (uint32_t j = 0; j < height; j++) {
        for (uint32_t i = 0; i < width; i++) {
            fb_draw_pixel(x + i, y + j, color);
        }
    }
}

void fb_draw_circle(uint32_t x0, uint32_t y0, uint32_t radius, uint32_t color) {
    // Bresenham's circle algorithm
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        fb_draw_pixel(x0 + x, y0 + y, color);
        fb_draw_pixel(x0 + y, y0 + x, color);
        fb_draw_pixel(x0 - y, y0 + x, color);
        fb_draw_pixel(x0 - x, y0 + y, color);
        fb_draw_pixel(x0 - x, y0 - y, color);
        fb_draw_pixel(x0 - y, y0 - x, color);
        fb_draw_pixel(x0 + y, y0 - x, color);
        fb_draw_pixel(x0 + x, y0 - y, color);
        
        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

void fb_fill_circle(uint32_t x0, uint32_t y0, uint32_t radius, uint32_t color) {
    // Modified Bresenham's circle algorithm for filled circles
    int x = radius;
    int y = 0;
    int err = 0;
    
    while (x >= y) {
        // Draw horizontal lines across the circle
        for (int i = -x; i <= x; i++) {
            fb_draw_pixel(x0 + i, y0 + y, color);
            fb_draw_pixel(x0 + i, y0 - y, color);
        }
        for (int i = -y; i <= y; i++) {
            fb_draw_pixel(x0 + i, y0 + x, color);
            fb_draw_pixel(x0 + i, y0 - x, color);
        }
        
        y++;
        if (err <= 0) {
            err += 2 * y + 1;
        }
        if (err > 0) {
            x--;
            err -= 2 * x + 1;
        }
    }
}

void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    // Simple font implementation (8x8 pixels per character)
    // This is a basic implementation - a real one would use a proper font
    
    // Basic bitmap font (ASCII 32-127)
    // Each character is 8x8 pixels, represented as 8 bytes
    // Each byte represents a row, with 1 bits being the character pixels
    static const uint8_t font8x8[96][8] = {
        // Space (32)
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        // ! (33)
        {0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00},
        // Add more characters here...
        // For brevity, I'm only including a few characters
        // In a real implementation, you would include the full ASCII set
        
        // A (65)
        {0x3C, 0x42, 0x42, 0x7E, 0x42, 0x42, 0x42, 0x00},
        // B (66)
        {0x7C, 0x42, 0x42, 0x7C, 0x42, 0x42, 0x7C, 0x00},
        // C (67)
        {0x3C, 0x42, 0x40, 0x40, 0x40, 0x42, 0x3C, 0x00}
        // Add more characters as needed
    };
    
    // Ensure character is in range
    if (c < 32 || c > 127)
        c = '?';
    
    // Get the character bitmap
    const uint8_t* char_bitmap = font8x8[c - 32];
    
    // Draw the character pixel by pixel
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (char_bitmap[row] & (1 << (7 - col))) {
                fb_draw_pixel(x + col, y + row, color);
            }
        }
    }
}

void fb_draw_text(uint32_t x, uint32_t y, const char* text, uint32_t color) {
    uint32_t cursor_x = x;
    uint32_t cursor_y = y;
    
    while (*text) {
        if (*text == '\n') {
            // Newline
            cursor_x = x;
            cursor_y += 8;  // Assuming 8 pixel high font
        } else if (*text == '\r') {
            // Carriage return
            cursor_x = x;
        } else {
            // Draw the character
            fb_draw_char(cursor_x, cursor_y, *text, color);
            cursor_x += 8;  // Assuming 8 pixel wide font
        }
        
        text++;
    }
}

void fb_clear(uint32_t color) {
    fb_ioctl(&fb_device, FB_IOCTL_CLEAR, &color);
}

void fb_swap_buffers(void) {
    fb_ioctl(&fb_device, FB_IOCTL_SWAP_BUFFERS, NULL);
}

void fb_set_double_buffering(uint8_t enable) {
    fb_ioctl(&fb_device, FB_IOCTL_SET_DOUBLE_BUFFERING, &enable);
}

void fb_get_info(fb_info_t* info) {
    fb_ioctl(&fb_device, FB_IOCTL_GET_INFO, info);
}

void fb_set_resolution(uint32_t width, uint32_t height, uint8_t bits_per_pixel) {
    fb_resolution_t res;
    res.width = width;
    res.height = height;
    res.bits_per_pixel = bits_per_pixel;
    
    fb_ioctl(&fb_device, FB_IOCTL_SET_RESOLUTION, &res);
}

// Initialize and register framebuffer device
int hal_framebuffer_init(void) {
    // Setup device
    fb_device.type = HAL_DEVICE_FRAMEBUFFER;
    fb_device.mode = HAL_MODE_POLLING;
    fb_device.private_data = &fb_data;
    
    // Set functions
    fb_device.init = fb_init;
    fb_device.close = fb_close;
    fb_device.read = fb_read;
    fb_device.write = fb_write;
    fb_device.ioctl = fb_ioctl;
    
    // Register with HAL
    int result = hal_register_device(&fb_device);
    if (result != 0) {
        terminal_writestring("Failed to register framebuffer device\n");
        return result;
    }
    
    // Initialize the device
    result = fb_init(&fb_device);
    
    return result;
}