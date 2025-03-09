// src/gui/window.c
#include "gui/window.h"
#include "hal_framebuffer.h"
#include "hal_mouse.h"
#include "kmalloc.h"
#include "string.h"
#include "stdio.h"
#include "terminal.h"

// Window manager data
static window_t* windows[MAX_WINDOWS];
static uint32_t window_count = 0;
static uint32_t next_window_id = 1;
static window_t* active_window = NULL;
static window_t* drag_window = NULL;
static uint8_t wm_initialized = 0;

// UI metrics
#define TITLE_BAR_HEIGHT 20
#define BORDER_WIDTH 3
#define BUTTON_WIDTH 16
#define MIN_WINDOW_WIDTH 100
#define MIN_WINDOW_HEIGHT 50

// UI colors
#define COLOR_WINDOW_BG        FB_COLOR_LIGHT_GRAY
#define COLOR_WINDOW_FG        FB_COLOR_BLACK
#define COLOR_WINDOW_BORDER    FB_COLOR_DARK_GRAY
#define COLOR_TITLE_ACTIVE_BG  FB_COLOR_BLUE
#define COLOR_TITLE_ACTIVE_FG  FB_COLOR_WHITE
#define COLOR_TITLE_INACTIVE_BG FB_COLOR_GRAY
#define COLOR_TITLE_INACTIVE_FG FB_COLOR_DARK_GRAY
#define COLOR_BUTTON_BG        FB_COLOR_LIGHT_GRAY
#define COLOR_BUTTON_FG        FB_COLOR_BLACK
#define COLOR_BUTTON_HIGHLIGHT FB_COLOR_WHITE
#define COLOR_BUTTON_SHADOW    FB_COLOR_DARK_GRAY
#define COLOR_DESKTOP_BG       0xFF006060  // Teal

// Message queue
#define MAX_MESSAGES 64
static window_message_t message_queue[MAX_MESSAGES];
static uint32_t message_head = 0;
static uint32_t message_tail = 0;
static uint32_t message_count = 0;

// Forward declarations
static void draw_window(window_t* window);
static void draw_title_bar(window_t* window);
static void draw_borders(window_t* window);
static void draw_client_area(window_t* window);
static int handle_mouse_event(mouse_event_t* event);
static window_t* find_window_at(int16_t x, int16_t y);
static int is_point_in_title_bar(window_t* window, int16_t x, int16_t y);
static int is_point_in_client_area(window_t* window, int16_t x, int16_t y);
static int is_point_in_resize_area(window_t* window, int16_t x, int16_t y, uint8_t* region);
static void bring_window_to_front(window_t* window);
static void update_window_z_order(void);
static int process_next_message(void);

// Initialize the window manager
int wm_init(void) {
    fb_info_t info;
    
    // Check if framebuffer is initialized
    fb_get_info(&info);
    if (info.width == 0 || info.height == 0) {
        terminal_writestring("Window Manager Error: Framebuffer not initialized\n");
        return -1;
    }
    
    // Initialize window array
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i] = NULL;
    }
    
    // Initialize message queue
    message_head = 0;
    message_tail = 0;
    message_count = 0;
    
    // Register mouse handler
    mouse_register_handler(handle_mouse_event);
    
    // Enable double buffering
    fb_set_double_buffering(1);
    
    // Clear the screen with desktop background color
    fb_clear(COLOR_DESKTOP_BG);
    fb_swap_buffers();
    
    wm_initialized = 1;
    
    terminal_writestring("Window Manager initialized\n");
    return 0;
}

// Process window manager events (mouse, keyboard, etc.)
void wm_process_events(void) {
    // Poll for mouse updates
    mouse_update();
    
    // Process messages in the queue
    while (process_next_message());
}

// Update the window manager
void wm_update(void) {
    if (!wm_initialized) return;
    
    // Clear the framebuffer with desktop background
    fb_clear(COLOR_DESKTOP_BG);
    
    // Draw all windows in z-order (back to front)
    for (int z = MAX_WINDOWS - 1; z >= 0; z--) {
        for (uint32_t i = 0; i < window_count; i++) {
            if (windows[i] && windows[i]->z_order == z && windows[i]->visible) {
                draw_window(windows[i]);
            }
        }
    }
    
    // Swap buffers to display the updated screen
    fb_swap_buffers();
}

// Update a specific region of the screen
void wm_update_region(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    // For simplicity, we'll just update the entire screen for now
    wm_update();
}

// Redraw all windows
void wm_redraw_all(void) {
    wm_update();
}

// Create a new window
window_t* window_create(const char* title, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t style) {
    if (window_count >= MAX_WINDOWS) {
        terminal_writestring("Window Manager Error: Maximum number of windows reached\n");
        return NULL;
    }
    
    // Allocate window structure
    window_t* window = kmalloc(sizeof(window_t));
    if (!window) {
        terminal_writestring("Window Manager Error: Failed to allocate window\n");
        return NULL;
    }
    
    // Initialize window data
    memset(window, 0, sizeof(window_t));
    
    window->id = next_window_id++;
    strncpy(window->title, title, MAX_WINDOW_TITLE - 1);
    window->x = x;
    window->y = y;
    window->width = width < MIN_WINDOW_WIDTH ? MIN_WINDOW_WIDTH : width;
    window->height = height < MIN_WINDOW_HEIGHT ? MIN_WINDOW_HEIGHT : height;
    window->style = style;
    window->state = WINDOW_STATE_NORMAL;
    window->z_order = 0;  // Will be updated later
    window->visible = 0;  // Not visible by default
    window->active = 0;   // Not active by default
    window->min_width = MIN_WINDOW_WIDTH;
    window->min_height = MIN_WINDOW_HEIGHT;
    window->max_width = 0xFFFFFFFF;  // No limit
    window->max_height = 0xFFFFFFFF; // No limit
    
    // Set default colors
    window->bg_color = COLOR_WINDOW_BG;
    window->fg_color = COLOR_WINDOW_FG;
    window->border_color = COLOR_WINDOW_BORDER;
    window->title_bg_color = COLOR_TITLE_INACTIVE_BG;
    window->title_fg_color = COLOR_TITLE_INACTIVE_FG;
    
    // Calculate client area
    if (!(style & WINDOW_STYLE_NOBORDER)) {
        window->client_x = BORDER_WIDTH;
        window->client_y = (style & WINDOW_STYLE_NOTITLE) ? BORDER_WIDTH : (BORDER_WIDTH + TITLE_BAR_HEIGHT);
        window->client_width = window->width - 2 * BORDER_WIDTH;
        window->client_height = window->height - window->client_y - BORDER_WIDTH;
    } else {
        window->client_x = 0;
        window->client_y = (style & WINDOW_STYLE_NOTITLE) ? 0 : TITLE_BAR_HEIGHT;
        window->client_width = window->width;
        window->client_height = window->height - window->client_y;
    }
    
    // Add to window array
    for (uint32_t i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == NULL) {
            windows[i] = window;
            window_count++;
            break;
        }
    }
    
    // Update z-order
    update_window_z_order();
    
    // Send create message to window
    window_send_message(window, WM_CREATE, 0, 0);
    
    return window;
}

// Destroy a window
void window_destroy(window_t* window) {
    if (!window) return;
    
    // Send destroy message to window
    window_send_message(window, WM_DESTROY, 0, 0);
    
    // Free window buffer if allocated
    if (window->buffer) {
        kfree(window->buffer);
    }
    
    // Remove from window array
    for (uint32_t i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] == window) {
            windows[i] = NULL;
            window_count--;
            break;
        }
    }
    
    // Update active window if needed
    if (active_window == window) {
        active_window = NULL;
        
        // Find new active window
        for (int z = 0; z < MAX_WINDOWS; z++) {
            for (uint32_t i = 0; i < MAX_WINDOWS; i++) {
                if (windows[i] && windows[i]->z_order == z && windows[i]->visible) {
                    active_window = windows[i];
                    active_window->active = 1;
                    active_window->title_bg_color = COLOR_TITLE_ACTIVE_BG;
                    active_window->title_fg_color = COLOR_TITLE_ACTIVE_FG;
                    window_send_message(active_window, WM_ACTIVATE, 1, 0);
                    break;
                }
            }
            if (active_window) break;
        }
    }
    
    // Free window structure
    kfree(window);
    
    // Update display
    wm_update();
}

// Show a window
void window_show(window_t* window) {
    if (!window) return;
    
    if (!window->visible) {
        window->visible = 1;
        bring_window_to_front(window);
        wm_update();
    }
}

// Hide a window
void window_hide(window_t* window) {
    if (!window) return;
    
    if (window->visible) {
        window->visible = 0;
        
        // If this was the active window, find new active window
        if (active_window == window) {
            active_window = NULL;
            
            // Deactivate this window
            window->active = 0;
            window->title_bg_color = COLOR_TITLE_INACTIVE_BG;
            window->title_fg_color = COLOR_TITLE_INACTIVE_FG;
            window_send_message(window, WM_ACTIVATE, 0, 0);
            
            // Find new active window
            for (int z = 0; z < MAX_WINDOWS; z++) {
                for (uint32_t i = 0; i < MAX_WINDOWS; i++) {
                    if (windows[i] && windows[i]->z_order == z && windows[i]->visible) {
                        active_window = windows[i];
                        active_window->active = 1;
                        active_window->title_bg_color = COLOR_TITLE_ACTIVE_BG;
                        active_window->title_fg_color = COLOR_TITLE_ACTIVE_FG;
                        window_send_message(active_window, WM_ACTIVATE, 1, 0);
                        break;
                    }
                }
                if (active_window) break;
            }
        }
        
        wm_update();
    }
}

// Move a window
void window_move(window_t* window, uint32_t x, uint32_t y) {
    if (!window) return;
    
    if (window->x != x || window->y != y) {
        window->x = x;
        window->y = y;
        window_send_message(window, WM_MOVE, x, y);
        wm_update();
    }
}

// Resize a window
void window_resize(window_t* window, uint32_t width, uint32_t height) {
    if (!window) return;
    
    // Enforce minimum size
    if (width < window->min_width) width = window->min_width;
    if (height < window->min_height) height = window->min_height;
    
    // Enforce maximum size
    if (width > window->max_width) width = window->max_width;
    if (height > window->max_height) height = window->max_height;
    
    if (window->width != width || window->height != height) {
        window->width = width;
        window->height = height;
        
        // Recalculate client area
        if (!(window->style & WINDOW_STYLE_NOBORDER)) {
            window->client_width = window->width - 2 * BORDER_WIDTH;
            window->client_height = window->height - window->client_y - BORDER_WIDTH;
        } else {
            window->client_width = window->width;
            window->client_height = window->height - window->client_y;
        }
        
        window_send_message(window, WM_SIZE, width, height);
        wm_update();
    }
}

// Set window title
void window_set_title(window_t* window, const char* title) {
    if (!window || !title) return;
    
    strncpy(window->title, title, MAX_WINDOW_TITLE - 1);
    window->title[MAX_WINDOW_TITLE - 1] = '\0';
    wm_update();
}

// Set window style
void window_set_style(window_t* window, uint32_t style) {
    if (!window) return;
    
    if (window->style != style) {
        window->style = style;
        
        // Recalculate client area
        if (!(style & WINDOW_STYLE_NOBORDER)) {
            window->client_x = BORDER_WIDTH;
            window->client_y = (style & WINDOW_STYLE_NOTITLE) ? BORDER_WIDTH : (BORDER_WIDTH + TITLE_BAR_HEIGHT);
            window->client_width = window->width - 2 * BORDER_WIDTH;
            window->client_height = window->height - window->client_y - BORDER_WIDTH;
        } else {
            window->client_x = 0;
            window->client_y = (style & WINDOW_STYLE_NOTITLE) ? 0 : TITLE_BAR_HEIGHT;
            window->client_width = window->width;
            window->client_height = window->height - window->client_y;
        }
        
        wm_update();
    }
}

// Set window state
void window_set_state(window_t* window, uint32_t state) {
    if (!window) return;
    
    if (window->state != state) {
        switch (state) {
            case WINDOW_STATE_NORMAL:
                // Restore window from minimized/maximized state
                // In a real implementation, we'd store the previous size/position
                window->state = WINDOW_STATE_NORMAL;
                break;
                
            case WINDOW_STATE_MINIMIZED:
                // Minimize window
                if (!(window->style & WINDOW_STYLE_NOMINIMIZE)) {
                    window->state = WINDOW_STATE_MINIMIZED;
                    window_hide(window);
                }
                break;
                
            case WINDOW_STATE_MAXIMIZED:
                // Maximize window
                if (!(window->style & WINDOW_STYLE_NOMAXIMIZE)) {
                    fb_info_t info;
                    fb_get_info(&info);
                    
                    window->state = WINDOW_STATE_MAXIMIZED;
                    
                    // Store original position and size for restore
                    // In a real implementation, we'd store these values in the window structure
                    
                    // Set window to cover the entire screen
                    window_move(window, 0, 0);
                    window_resize(window, info.width, info.height);
                }
                break;
                
            case WINDOW_STATE_HIDDEN:
                window_hide(window);
                break;
        }
        
        wm_update();
    }
}

// Activate a window
void window_activate(window_t* window) {
    if (!window) return;
    
    // Ignore if already active
    if (window == active_window) return;
    
    // Deactivate current active window
    if (active_window) {
        active_window->active = 0;
        active_window->title_bg_color = COLOR_TITLE_INACTIVE_BG;
        active_window->title_fg_color = COLOR_TITLE_INACTIVE_FG;
        window_send_message(active_window, WM_ACTIVATE, 0, 0);
    }
    
    // Set new active window
    active_window = window;
    window->active = 1;
    window->title_bg_color = COLOR_TITLE_ACTIVE_BG;
    window->title_fg_color = COLOR_TITLE_ACTIVE_FG;
    
    // Bring to front
    bring_window_to_front(window);
    
    // Send activation message
    window_send_message(window, WM_ACTIVATE, 1, 0);
    
    // Update display
    wm_update();
}

// Set window event handler
void window_set_event_handler(window_t* window, window_event_handler_t handler) {
    if (!window) return;
    
    window->event_handler = handler;
}

// Invalidate the entire window (mark for redraw)
void window_invalidate(window_t* window) {
    if (!window) return;
    
    // For now, just update the whole screen
    wm_update();
}

// Invalidate a region of the window
void window_invalidate_region(window_t* window, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (!window) return;
    
    // For now, just update the whole screen
    wm_update();
}

// Send a message to a window and process it immediately
int window_send_message(window_t* window, uint32_t msg_type, uint32_t param1, uint32_t param2) {
    if (!window) return 0;
    
    window_message_t msg;
    msg.type = msg_type;
    msg.param1 = param1;
    msg.param2 = param2;
    msg.window = window;
    
    // Call the window's event handler if available
    if (window->event_handler) {
        return window->event_handler(window, &msg);
    }
    
    return 0;
}

// Post a message to the message queue
int window_post_message(window_t* window, uint32_t msg_type, uint32_t param1, uint32_t param2) {
    if (!window) return 0;
    
    // Check if message queue is full
    if (message_count >= MAX_MESSAGES) {
        return 0;
    }
    
    // Create message
    window_message_t msg;
    msg.type = msg_type;
    msg.param1 = param1;
    msg.param2 = param2;
    msg.window = window;
    
    // Add to queue
    message_queue[message_tail] = msg;
    message_tail = (message_tail + 1) % MAX_MESSAGES;
    message_count++;
    
    return 1;
}

// Window drawing functions

void window_draw_pixel(window_t* window, uint32_t x, uint32_t y, uint32_t color) {
    if (!window) return;
    
    // Convert to screen coordinates
    uint32_t screen_x = window->x + window->client_x + x;
    uint32_t screen_y = window->y + window->client_y + y;
    
    // Draw pixel to framebuffer
    fb_draw_pixel(screen_x, screen_y, color);
}

void window_draw_line(window_t* window, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color) {
    if (!window) return;
    
    // Convert to screen coordinates
    uint32_t screen_x1 = window->x + window->client_x + x1;
    uint32_t screen_y1 = window->y + window->client_y + y1;
    uint32_t screen_x2 = window->x + window->client_x + x2;
    uint32_t screen_y2 = window->y + window->client_y + y2;
    
    // Draw line to framebuffer
    fb_draw_line(screen_x1, screen_y1, screen_x2, screen_y2, color);
}

void window_draw_rect(window_t* window, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    if (!window) return;
    
    // Convert to screen coordinates
    uint32_t screen_x = window->x + window->client_x + x;
    uint32_t screen_y = window->y + window->client_y + y;
    
    // Draw rectangle to framebuffer
    fb_draw_rect(screen_x, screen_y, width, height, color);
}

void window_fill_rect(window_t* window, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color) {
    if (!window) return;
    
    // Convert to screen coordinates
    uint32_t screen_x = window->x + window->client_x + x;
    uint32_t screen_y = window->y + window->client_y + y;
    
    // Draw filled rectangle to framebuffer
    fb_fill_rect(screen_x, screen_y, width, height, color);
}

void window_draw_circle(window_t* window, uint32_t x0, uint32_t y0, uint32_t radius, uint32_t color) {
    if (!window) return;
    
    // Convert to screen coordinates
    uint32_t screen_x = window->x + window->client_x + x0;
    uint32_t screen_y = window->y + window->client_y + y0;
    
    // Draw circle to framebuffer
    fb_draw_circle(screen_x, screen_y, radius, color);
}

void window_fill_circle(window_t* window, uint32_t x0, uint32_t y0, uint32_t radius, uint32_t color) {
    if (!window) return;
    
    // Convert to screen coordinates
    uint32_t screen_x = window->x + window->client_x + x0;
    uint32_t screen_y = window->y + window->client_y + y0;
    
    // Draw filled circle to framebuffer
    fb_fill_circle(screen_x, screen_y, radius, color);
}

void window_draw_text(window_t* window, uint32_t x, uint32_t y, const char* text, uint32_t color) {
    if (!window || !text) return;
    
    // Convert to screen coordinates
    uint32_t screen_x = window->x + window->client_x + x;
    uint32_t screen_y = window->y + window->client_y + y;
    
    // Draw text to framebuffer
    fb_draw_text(screen_x, screen_y, text, color);
}

void window_clear(window_t* window, uint32_t color) {
    if (!window) return;
    
    window_fill_rect(window, 0, 0, window->client_width, window->client_height, color);
}

// Coordinate conversion functions

void window_client_to_screen(window_t* window, uint32_t client_x, uint32_t client_y, uint32_t* screen_x, uint32_t* screen_y) {
    if (!window || !screen_x || !screen_y) return;
    
    *screen_x = window->x + window->client_x + client_x;
    *screen_y = window->y + window->client_y + client_y;
}

void window_screen_to_client(window_t* window, uint32_t screen_x, uint32_t screen_y, uint32_t* client_x, uint32_t* client_y) {
    if (!window || !client_x || !client_y) return;
    
    if (screen_x >= window->x + window->client_x && 
        screen_y >= window->y + window->client_y &&
        screen_x < window->x + window->client_x + window->client_width &&
        screen_y < window->y + window->client_y + window->client_height) {
        
        *client_x = screen_x - (window->x + window->client_x);
        *client_y = screen_y - (window->y + window->client_y);
    } else {
        *client_x = 0;
        *client_y = 0;
    }
}

// Internal window manager functions

// Draw a window
static void draw_window(window_t* window) {
    if (!window || !window->visible) return;
    
    // Draw borders first
    if (!(window->style & WINDOW_STYLE_NOBORDER)) {
        draw_borders(window);
    }
    
    // Draw title bar if needed
    if (!(window->style & WINDOW_STYLE_NOTITLE)) {
        draw_title_bar(window);
    }
    
    // Draw client area
    draw_client_area(window);
    
    // Send paint message to window
    window_send_message(window, WM_PAINT, 0, 0);
}

// Draw window title bar
static void draw_title_bar(window_t* window) {
    if (!window || (window->style & WINDOW_STYLE_NOTITLE)) return;
    
    uint32_t title_x = window->x;
    uint32_t title_y = window->y;
    uint32_t title_width = window->width;
    uint32_t title_height = TITLE_BAR_HEIGHT;
    
    // Draw title bar background
    fb_fill_rect(title_x, title_y, title_width, title_height, window->title_bg_color);
    
    // Draw title text
    if (window->title[0] != '\0') {
        fb_draw_text(title_x + 5, title_y + 5, window->title, window->title_fg_color);
    }
    
    // Draw window control buttons (close, maximize, minimize)
    uint32_t button_x = title_x + title_width - BUTTON_WIDTH - 2;
    uint32_t button_y = title_y + 2;
    
    // Close button
    if (!(window->style & WINDOW_STYLE_NOCLOSE)) {
        fb_fill_rect(button_x, button_y, BUTTON_WIDTH, BUTTON_WIDTH, FB_COLOR_RED);
        // Draw X
        fb_draw_line(button_x + 3, button_y + 3, button_x + BUTTON_WIDTH - 3, button_y + BUTTON_WIDTH - 3, FB_COLOR_WHITE);
        fb_draw_line(button_x + BUTTON_WIDTH - 3, button_y + 3, button_x + 3, button_y + BUTTON_WIDTH - 3, FB_COLOR_WHITE);
        button_x -= BUTTON_WIDTH + 2;
    }
    
    // Maximize button
    if (!(window->style & WINDOW_STYLE_NOMAXIMIZE)) {
        fb_fill_rect(button_x, button_y, BUTTON_WIDTH, BUTTON_WIDTH, window->title_bg_color);
        fb_draw_rect(button_x + 3, button_y + 3, BUTTON_WIDTH - 6, BUTTON_WIDTH - 6, window->title_fg_color);
        button_x -= BUTTON_WIDTH + 2;
    }
    
    // Minimize button
    if (!(window->style & WINDOW_STYLE_NOMINIMIZE)) {
        fb_fill_rect(button_x, button_y, BUTTON_WIDTH, BUTTON_WIDTH, window->title_bg_color);
        fb_draw_line(button_x + 3, button_y + BUTTON_WIDTH - 3, button_x + BUTTON_WIDTH - 3, button_y + BUTTON_WIDTH - 3, window->title_fg_color);
    }
}

// Draw window borders
static void draw_borders(window_t* window) {
    if (!window || (window->style & WINDOW_STYLE_NOBORDER)) return;
    
    uint32_t x = window->x;
    uint32_t y = window->y;
    uint32_t width = window->width;
    uint32_t height = window->height;
    
    // Draw borders
    // Top border (already drawn if title bar exists)
    if (window->style & WINDOW_STYLE_NOTITLE) {
        fb_fill_rect(x, y, width, BORDER_WIDTH, window->border_color);
    }
    
    // Bottom border
    fb_fill_rect(x, y + height - BORDER_WIDTH, width, BORDER_WIDTH, window->border_color);
    
    // Left border
    fb_fill_rect(x, y, BORDER_WIDTH, height, window->border_color);
    
    // Right border
    fb_fill_rect(x + width - BORDER_WIDTH, y, BORDER_WIDTH, height, window->border_color);
}

// Draw the client area of a window
static void draw_client_area(window_t* window) {
    if (!window) return;
    
    uint32_t client_x = window->x + window->client_x;
    uint32_t client_y = window->y + window->client_y;
    uint32_t client_width = window->client_width;
    uint32_t client_height = window->client_height;
    
    // Fill client area with background color
    fb_fill_rect(client_x, client_y, client_width, client_height, window->bg_color);
}

// Handle mouse events
static int handle_mouse_event(mouse_event_t* event) {
    if (!event) return 0;
    
    // Get mouse state
    int16_t x = event->x;
    int16_t y = event->y;
    uint8_t buttons = event->buttons;
    uint8_t prev_buttons = event->prev_buttons;
    int16_t dx = event->dx;
    int16_t dy = event->dy;
    
    // Handle window dragging
    if (drag_window) {
        if (!(buttons & MOUSE_BUTTON_LEFT)) {
            // Button released, end drag
            drag_window->dragging = 0;
            drag_window->resizing = 0;
            drag_window = NULL;
        } else {
            // Continue dragging
            if (drag_window->dragging) {
                // Move window
                uint32_t new_x = x - drag_window->drag_offset_x;
                uint32_t new_y = y - drag_window->drag_offset_y;
                window_move(drag_window, new_x, new_y);
            } else if (drag_window->resizing) {
                // Resize window
                uint32_t new_width, new_height;
                
                switch (drag_window->drag_region) {
                    case 1: // Top-left
                        new_width = drag_window->width + (drag_window->x - x);
                        new_height = drag_window->height + (drag_window->y - y);
                        window_move(drag_window, x, y);
                        window_resize(drag_window, new_width, new_height);
                        break;
                    case 2: // Top
                        new_height = drag_window->height + (drag_window->y - y);
                        window_move(drag_window, drag_window->x, y);
                        window_resize(drag_window, drag_window->width, new_height);
                        break;
                    case 3: // Top-right
                        new_width = drag_window->width + (x - (drag_window->x + drag_window->width));
                        new_height = drag_window->height + (drag_window->y - y);
                        window_move(drag_window, drag_window->x, y);
                        window_resize(drag_window, new_width, new_height);
                        break;
                    case 4: // Right
                        new_width = drag_window->width + (x - (drag_window->x + drag_window->width));
                        window_resize(drag_window, new_width, drag_window->height);
                        break;
                    case 5: // Bottom-right
                        new_width = drag_window->width + (x - (drag_window->x + drag_window->width));
                        new_height = drag_window->height + (y - (drag_window->y + drag_window->height));
                        window_resize(drag_window, new_width, new_height);
                        break;
                    case 6: // Bottom
                        new_height = drag_window->height + (y - (drag_window->y + drag_window->height));
                        window_resize(drag_window, drag_window->width, new_height);
                        break;
                    case 7: // Bottom-left
                        new_width = drag_window->width + (drag_window->x - x);
                        new_height = drag_window->height + (y - (drag_window->y + drag_window->height));
                        window_move(drag_window, x, drag_window->y);
                        window_resize(drag_window, new_width, new_height);
                        break;
                    case 8: // Left
                        new_width = drag_window->width + (drag_window->x - x);
                        window_move(drag_window, x, drag_window->y);
                        window_resize(drag_window, new_width, drag_window->height);
                        break;
                }
            }
        }
        
        return 1;
    }
    
    // Find window under mouse cursor
    window_t* window = find_window_at(x, y);
    
    // Handle mouse button press
    if ((buttons & MOUSE_BUTTON_LEFT) && !(prev_buttons & MOUSE_BUTTON_LEFT)) {
        // Left button pressed
        if (window) {
            // Bring window to front and make it active
            window_activate(window);
            
            uint8_t resize_region = 0;
            
            // Check if clicking on title bar
            if (is_point_in_title_bar(window, x, y)) {
                // Check close button
                if (!(window->style & WINDOW_STYLE_NOCLOSE)) {
                    uint32_t button_x = window->x + window->width - BUTTON_WIDTH - 2;
                    uint32_t button_y = window->y + 2;
                    
                    if (x >= button_x && x < button_x + BUTTON_WIDTH &&
                        y >= button_y && y < button_y + BUTTON_WIDTH) {
                        // Close button clicked
                        window_send_message(window, WM_CLOSE, 0, 0);
                        return 1;
                    }
                    
                    button_x -= BUTTON_WIDTH + 2;
                }
                
                // Check maximize button
                if (!(window->style & WINDOW_STYLE_NOMAXIMIZE)) {
                    uint32_t button_x = window->x + window->width - (BUTTON_WIDTH + 2) * 
                                       ((!(window->style & WINDOW_STYLE_NOCLOSE)) ? 2 : 1);
                    uint32_t button_y = window->y + 2;
                    
                    if (x >= button_x && x < button_x + BUTTON_WIDTH &&
                        y >= button_y && y < button_y + BUTTON_WIDTH) {
                        // Maximize/restore button clicked
                        if (window->state == WINDOW_STATE_MAXIMIZED) {
                            window_set_state(window, WINDOW_STATE_NORMAL);
                        } else {
                            window_set_state(window, WINDOW_STATE_MAXIMIZED);
                        }
                        return 1;
                    }
                    
                    button_x -= BUTTON_WIDTH + 2;
                }
                
                // Check minimize button
                if (!(window->style & WINDOW_STYLE_NOMINIMIZE)) {
                    uint32_t button_x = window->x + window->width - (BUTTON_WIDTH + 2) * 
                                       ((!(window->style & WINDOW_STYLE_NOCLOSE)) ? 3 : 2);
                    uint32_t button_y = window->y + 2;
                    
                    if (x >= button_x && x < button_x + BUTTON_WIDTH &&
                        y >= button_y && y < button_y + BUTTON_WIDTH) {
                        // Minimize button clicked
                        window_set_state(window, WINDOW_STATE_MINIMIZED);
                        return 1;
                    }
                }
                
                // Start dragging
                window->dragging = 1;
                window->drag_offset_x = x - window->x;
                window->drag_offset_y = y - window->y;
                drag_window = window;
            } 
            // Check if clicking on resize area
            else if (!(window->style & WINDOW_STYLE_NORESIZE) && 
                    is_point_in_resize_area(window, x, y, &resize_region)) {
                // Start resizing
                window->resizing = 1;
                window->drag_region = resize_region;
                drag_window = window;
            }
            // Check if clicking in client area
            else if (is_point_in_client_area(window, x, y)) {
                // Convert to client coordinates
                uint32_t client_x, client_y;
                window_screen_to_client(window, x, y, &client_x, &client_y);
                
                // Send mouse down message
                window_send_message(window, WM_MOUSEDOWN, 
                                   MOUSE_BUTTON_LEFT | (client_x << 16),
                                   client_y);
            }
        }
    } 
    // Handle mouse button release
    else if (!(buttons & MOUSE_BUTTON_LEFT) && (prev_buttons & MOUSE_BUTTON_LEFT)) {
        // Left button released
        if (window) {
            // Convert to client coordinates
            uint32_t client_x, client_y;
            window_screen_to_client(window, x, y, &client_x, &client_y);
            
            // Send mouse up message
            window_send_message(window, WM_MOUSEUP, 
                               MOUSE_BUTTON_LEFT | (client_x << 16),
                               client_y);
        }
    }
    
    // Handle mouse movement
    if (dx != 0 || dy != 0) {
        if (window) {
            // Check if mouse is in client area
            if (is_point_in_client_area(window, x, y)) {
                // Convert to client coordinates
                uint32_t client_x, client_y;
                window_screen_to_client(window, x, y, &client_x, &client_y);
                
                // Send mouse move message
                window_send_message(window, WM_MOUSEMOVE, client_x, client_y);
            }
        }
    }
    
    return 0;
}

// Find the window at a given screen position
static window_t* find_window_at(int16_t x, int16_t y) {
    // Iterate through windows in z-order (front to back)
    for (int z = 0; z < MAX_WINDOWS; z++) {
        for (uint32_t i = 0; i < MAX_WINDOWS; i++) {
            window_t* window = windows[i];
            
            if (window && window->z_order == z && window->visible) {
                // Check if point is within window
                if (x >= window->x && x < window->x + window->width &&
                    y >= window->y && y < window->y + window->height) {
                    return window;
                }
            }
        }
    }
    
    return NULL;
}

// Check if point is in the title bar
static int is_point_in_title_bar(window_t* window, int16_t x, int16_t y) {
    if (!window || (window->style & WINDOW_STYLE_NOTITLE)) return 0;
    
    return (x >= window->x && x < window->x + window->width &&
            y >= window->y && y < window->y + TITLE_BAR_HEIGHT);
}

// Check if point is in the client area
static int is_point_in_client_area(window_t* window, int16_t x, int16_t y) {
    if (!window) return 0;
    
    return (x >= window->x + window->client_x && 
            x < window->x + window->client_x + window->client_width &&
            y >= window->y + window->client_y && 
            y < window->y + window->client_y + window->client_height);
}

// Check if point is in resize area and which area it's in
static int is_point_in_resize_area(window_t* window, int16_t x, int16_t y, uint8_t* region) {
    if (!window || (window->style & WINDOW_STYLE_NORESIZE)) return 0;
    
    // Border width for resize
    int border = BORDER_WIDTH + 2;
    
    // Check corners first (they take precedence)
    // Top-left
    if (x >= window->x && x < window->x + border &&
        y >= window->y && y < window->y + border) {
        if (region) *region = 1;
        return 1;
    }
    
    // Top-right
    if (x >= window->x + window->width - border && x < window->x + window->width &&
        y >= window->y && y < window->y + border) {
        if (region) *region = 3;
        return 1;
    }
    
    // Bottom-right
    if (x >= window->x + window->width - border && x < window->x + window->width &&
        y >= window->y + window->height - border && y < window->y + window->height) {
        if (region) *region = 5;
        return 1;
    }
    
    // Bottom-left
    if (x >= window->x && x < window->x + border &&
        y >= window->y + window->height - border && y < window->y + window->height) {
        if (region) *region = 7;
        return 1;
    }
    
    // Then check edges
    // Top
    if (x >= window->x + border && x < window->x + window->width - border &&
        y >= window->y && y < window->y + border &&
        !(window->style & WINDOW_STYLE_NOTITLE)) {  // Don't resize from top if has title bar
        if (region) *region = 2;
        return 1;
    }
    
    // Right
    if (x >= window->x + window->width - border && x < window->x + window->width &&
        y >= window->y + border && y < window->y + window->height - border) {
        if (region) *region = 4;
        return 1;
    }
    
    // Bottom
    if (x >= window->x + border && x < window->x + window->width - border &&
        y >= window->y + window->height - border && y < window->y + window->height) {
        if (region) *region = 6;
        return 1;
    }
    
    // Left
    if (x >= window->x && x < window->x + border &&
        y >= window->y + border && y < window->y + window->height - border) {
        if (region) *region = 8;
        return 1;
    }
    
    return 0;
}

// Bring a window to the front
static void bring_window_to_front(window_t* window) {
    if (!window) return;
    
    // If already at front, do nothing
    if (window->z_order == 0) return;
    
    // Update z-order of all windows
    for (uint32_t i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i] && windows[i]->z_order < window->z_order) {
            windows[i]->z_order++;
        }
    }
    
    // Set this window as front
    window->z_order = 0;
}

// Update the z-order of all windows
static void update_window_z_order(void) {
    // Count windows
    uint32_t count = 0;
    for (uint32_t i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i]) {
            count++;
        }
    }
    
    // If no windows, nothing to do
    if (count == 0) return;
    
    // Sort windows by z-order
    for (uint32_t i = 0; i < MAX_WINDOWS; i++) {
        if (windows[i]) {
            windows[i]->z_order = count - 1;
            count--;
        }
    }
    
    // Set active window as front
    if (active_window) {
        bring_window_to_front(active_window);
    }
}

// Process the next message in the queue
static int process_next_message(void) {
    if (message_count == 0) {
        return 0;  // No messages
    }
    
    // Get message
    window_message_t msg = message_queue[message_head];
    message_head = (message_head + 1) % MAX_MESSAGES;
    message_count--;
    
    // Process message
    if (msg.window && msg.window->event_handler) {
        msg.window->event_handler(msg.window, &msg);
    }
    
    return 1;  // Message processed
}

// Control implementation

// Create a button control
window_control_t* control_create_button(window_t* parent, uint32_t id, const char* text, 
                                      uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (!parent) return NULL;
    
    // Allocate control structure
    window_control_t* control = kmalloc(sizeof(window_control_t));
    if (!control) {
        return NULL;
    }
    
    // Initialize control data
    memset(control, 0, sizeof(window_control_t));
    
    control->parent = parent;
    control->id = id;
    control->x = x;
    control->y = y;
    control->width = width;
    control->height = height;
    control->enabled = 1;
    control->visible = 1;
    control->focused = 0;
    
    // Set default colors
    control->bg_color = COLOR_BUTTON_BG;
    control->fg_color = COLOR_BUTTON_FG;
    control->border_color = COLOR_BUTTON_SHADOW;
    
    // Allocate memory for button text
    control->control_data = kmalloc(MAX_WINDOW_TITLE);
    if (control->control_data) {
        strncpy((char*)control->control_data, text, MAX_WINDOW_TITLE - 1);
        ((char*)control->control_data)[MAX_WINDOW_TITLE - 1] = '\0';
    }
    
    // Set default event handler
    // In a real implementation, we'd have specific handlers for each control type
    
    // Draw the button
    window_fill_rect(parent, x, y, width, height, control->bg_color);
    window_draw_rect(parent, x, y, width, height, control->border_color);
    
    // Center text in button
    if (control->control_data) {
        uint32_t text_len = strlen((char*)control->control_data);
        uint32_t text_x = x + (width - text_len * 8) / 2;  // Assuming 8 pixel wide characters
        uint32_t text_y = y + (height - 8) / 2;  // Assuming 8 pixel high characters
        
        window_draw_text(parent, text_x, text_y, (char*)control->control_data, control->fg_color);
    }
    
    return control;
}

// Create a label control
window_control_t* control_create_label(window_t* parent, uint32_t id, const char* text, 
                                     uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    if (!parent) return NULL;
    
    // Allocate control structure
    window_control_t* control = kmalloc(sizeof(window_control_t));
    if (!control) {
        return NULL;
    }
    
    // Initialize control data
    memset(control, 0, sizeof(window_control_t));
    
    control->parent = parent;
    control->id = id;
    control->x = x;
    control->y = y;
    control->width = width;
    control->height = height;
    control->enabled = 1;
    control->visible = 1;
    control->focused = 0;
    
    // Set default colors
    control->bg_color = parent->bg_color;
    control->fg_color = parent->fg_color;
    control->border_color = parent->border_color;
    
    // Allocate memory for label text
    control->control_data = kmalloc(MAX_WINDOW_TITLE);
    if (control->control_data) {
        strncpy((char*)control->control_data, text, MAX_WINDOW_TITLE - 1);
        ((char*)control->control_data)[MAX_WINDOW_TITLE - 1] = '\0';
    }
    
    // Draw the label
    window_draw_text(parent, x, y, (char*)control->control_data, control->fg_color);
    
    return control;
}

// For the sake of brevity, we'll implement the rest of the control functions as stubs

// Create a textbox control
window_control_t* control_create_textbox(window_t* parent, uint32_t id, const char* text, 
                                       uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    // Placeholder implementation
    window_control_t* control = kmalloc(sizeof(window_control_t));
    if (control) {
        memset(control, 0, sizeof(window_control_t));
        control->parent = parent;
        control->id = id;
        control->x = x;
        control->y = y;
        control->width = width;
        control->height = height;
        control->enabled = 1;
        control->visible = 1;
        
        // Draw a simple textbox
        window_fill_rect(parent, x, y, width, height, FB_COLOR_WHITE);
        window_draw_rect(parent, x, y, width, height, FB_COLOR_DARK_GRAY);
        if (text) {
            window_draw_text(parent, x + 3, y + 3, text, FB_COLOR_BLACK);
        }
    }
    return control;
}

// Create a checkbox control
window_control_t* control_create_checkbox(window_t* parent, uint32_t id, const char* text, 
                                        uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
                                        uint8_t checked) {
    // Placeholder implementation
    window_control_t* control = kmalloc(sizeof(window_control_t));
    if (control) {
        memset(control, 0, sizeof(window_control_t));
        control->parent = parent;
        control->id = id;
        control->x = x;
        control->y = y;
        control->width = width;
        control->height = height;
        control->enabled = 1;
        control->visible = 1;
        
        // Store checked state
        control->control_data = kmalloc(1);
        if (control->control_data) {
            *(uint8_t*)control->control_data = checked;
        }
        
        // Draw checkbox
        int box_size = 12;
        window_draw_rect(parent, x, y, box_size, box_size, FB_COLOR_DARK_GRAY);
        window_fill_rect(parent, x + 1, y + 1, box_size - 2, box_size - 2, FB_COLOR_WHITE);
        
        if (checked) {
            // Draw checkmark
            window_draw_line(parent, x + 2, y + 6, x + 5, y + 9, FB_COLOR_BLACK);
            window_draw_line(parent, x + 5, y + 9, x + 10, y + 2, FB_COLOR_BLACK);
        }
        
        // Draw label
        if (text) {
            window_draw_text(parent, x + box_size + 5, y + 2, text, FB_COLOR_BLACK);
        }
    }
    return control;
}

// Create a radio button control
window_control_t* control_create_radiobutton(window_t* parent, uint32_t id, const char* text, 
                                           uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
                                           uint8_t checked) {
    // Placeholder implementation
    window_control_t* control = kmalloc(sizeof(window_control_t));
    if (control) {
        memset(control, 0, sizeof(window_control_t));
        control->parent = parent;
        control->id = id;
        control->x = x;
        control->y = y;
        control->width = width;
        control->height = height;
        control->enabled = 1;
        control->visible = 1;
        
        // Store checked state
        control->control_data = kmalloc(1);
        if (control->control_data) {
            *(uint8_t*)control->control_data = checked;
        }
        
        // Draw radio button
        int radius = 6;
        window_draw_circle(parent, x + radius, y + radius, radius, FB_COLOR_DARK_GRAY);
        window_fill_circle(parent, x + radius, y + radius, radius - 1, FB_COLOR_WHITE);
        
        if (checked) {
            // Draw selected dot
            window_fill_circle(parent, x + radius, y + radius, radius - 3, FB_COLOR_BLACK);
        }
        
        // Draw label
        if (text) {
            window_draw_text(parent, x + radius * 2 + 5, y + 2, text, FB_COLOR_BLACK);
        }
    }
    return control;
}

// Create a listbox control
window_control_t* control_create_listbox(window_t* parent, uint32_t id, 
                                       uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    // Placeholder implementation
    window_control_t* control = kmalloc(sizeof(window_control_t));
    if (control) {
        memset(control, 0, sizeof(window_control_t));
        control->parent = parent;
        control->id = id;
        control->x = x;
        control->y = y;
        control->width = width;
        control->height = height;
        control->enabled = 1;
        control->visible = 1;
        
        // Draw listbox
        window_fill_rect(parent, x, y, width, height, FB_COLOR_WHITE);
        window_draw_rect(parent, x, y, width, height, FB_COLOR_DARK_GRAY);
    }
    return control;
}

// Destroy a control
void control_destroy(window_control_t* control) {
    if (!control) return;
    
    // Free control data
    if (control->control_data) {
        kfree(control->control_data);
    }
    
    // Free control structure
    kfree(control);
}

// Set control text
void control_set_text(window_control_t* control, const char* text) {
    if (!control || !text) return;
    
    // Assuming control_data is a character string
    if (control->control_data) {
        strncpy((char*)control->control_data, text, MAX_WINDOW_TITLE - 1);
        ((char*)control->control_data)[MAX_WINDOW_TITLE - 1] = '\0';
    }
    
    // Redraw control
    control_invalidate(control);
}

// Get control text
const char* control_get_text(window_control_t* control) {
    if (!control || !control->control_data) return NULL;
    
    return (const char*)control->control_data;
}

// Set control enabled state
void control_set_enabled(window_control_t* control, uint8_t enabled) {
    if (!control) return;
    
    control->enabled = enabled;
    
    // Redraw control
    control_invalidate(control);
}

// Set control visible state
void control_set_visible(window_control_t* control, uint8_t visible) {
    if (!control) return;
    
    control->visible = visible;
    
    // Redraw control
    control_invalidate(control);
}

// Set control focus
void control_set_focus(window_control_t* control) {
    if (!control || !control->enabled || !control->visible) return;
    
    // Remove focus from other controls
    window_control_t* other = NULL;
    // In a real implementation, we'd iterate through all controls
    
    // Set focus to this control
    control->focused = 1;
    
    // Redraw control
    control_invalidate(control);
}

// Invalidate a control
void control_invalidate(window_control_t* control) {
    if (!control) return;
    
    // For simplicity, just invalidate the parent window
    window_invalidate(control->parent);
}