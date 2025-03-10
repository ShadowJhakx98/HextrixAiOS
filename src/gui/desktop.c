// src/gui/desktop.c
#include "hal.h"
#include <stdio.h>
#include "gui/desktop.h"
#include "gui/window.h"
#include "hal_framebuffer.h"
#include "hal_mouse.h"
#include "hal_keyboard.h"
#include "hal_timer.h"
#include "terminal.h"
#include "kmalloc.h"
#include "string.h"

// Add these definitions to desktop.c
#define CURSOR_WIDTH 8
#define CURSOR_HEIGHT 12

// Maximum number of desktop icons
#define MAX_DESKTOP_ICONS 16

// Icon dimensions
#define ICON_WIDTH 50
#define ICON_HEIGHT 50
#define ICON_SPACING_X 80
#define ICON_SPACING_Y 80
#define ICON_MARGIN_X 20
#define ICON_MARGIN_Y 20

// Taskbar dimensions
#define TASKBAR_HEIGHT 30
#define TASKBAR_BUTTON_WIDTH 120
#define TASKBAR_BUTTON_HEIGHT 20
#define TASKBAR_BUTTON_MARGIN 5
#define TASKBAR_START_BUTTON_WIDTH 80

// Desktop themes
#define THEME_BLUE 0
#define THEME_GREEN 1
#define THEME_TEAL 2

// Desktop icons
static desktop_icon_t desktop_icons[MAX_DESKTOP_ICONS];
static uint32_t icon_count = 0;

// Desktop taskbar
static desktop_taskbar_t taskbar;

// Desktop background color
static uint32_t desktop_bg_color = FB_COLOR_BLUE;  // Use predefined blue instead of teal

// Current theme
static uint8_t current_theme = THEME_TEAL;

// Program windows
static window_t* file_browser_window = NULL;
static window_t* terminal_window = NULL;
static window_t* text_editor_window = NULL;
static window_t* settings_window = NULL;
static window_t* active_window = NULL;  // Currently active window

// Simple cursor shape data
static const uint8_t cursor_data[CURSOR_HEIGHT] = {
    0b10000000,  // X.......
    0b11000000,  // XX......
    0b11100000,  // XXX.....
    0b11110000,  // XXXX....
    0b11111000,  // XXXXX...
    0b11111100,  // XXXXXX..
    0b11111110,  // XXXXXXX.
    0b11111100,  // XXXXXX..
    0b11001100,  // XX..XX..
    0b10000110,  // X....XX.
    0b00000110,  // .....XX.
    0b00000011   // ......XX
};

// Forward declarations
static void draw_desktop(void);
static void draw_taskbar(void);
static void draw_icons(void);
static void desktop_mouse_handler(mouse_event_t* event);
static void update_clock(void);
static int welcome_window_handler(window_t* window, window_message_t* msg);
static int file_browser_event_handler(window_t* window, window_message_t* msg);
static int terminal_event_handler(window_t* window, window_message_t* msg);
static int text_editor_event_handler(window_t* window, window_message_t* msg);
static int settings_event_handler(window_t* window, window_message_t* msg);


// Last known mouse position for redrawing
static int16_t last_mouse_x = -1;
static int16_t last_mouse_y = -1;
static uint8_t mouse_visible = 1;
static uint32_t mouse_background[CURSOR_WIDTH * CURSOR_HEIGHT]; // Store background pixels


// Debug print function to help with diagnosis
// Debug print function to help with diagnosis
static void desktop_debug(const char* message) {
    // Skip most debug messages during normal operation to prevent screen flooding
    static int debug_count = 0;
    if (debug_count > 10) {
        return; // Limit debug messages after startup
    }
    debug_count++;
    
    // Use direct serial output if available, otherwise use terminal
    // This assumes serial_print is available or can be implemented
    #ifdef ENABLE_SERIAL_DEBUG
        serial_print("[DESKTOP] ");
        serial_print(message);
        serial_print("\n");
    #else
        // Only log critical messages to terminal
        if (debug_count <= 5) {
            terminal_writestring("[DESKTOP] ");
            terminal_writestring(message);
            terminal_writestring("\n");
        }
    #endif
}

// Initialize the desktop environment
int desktop_init(void) {
    terminal_writestring("desktop_init: Entering desktop_init()\n");

    // Initialize window manager
    terminal_writestring("desktop_init: Initializing window manager...\n");
    if (wm_init() != 0) {
        terminal_writestring("desktop_init: ERROR: Failed to initialize window manager!\n");
        terminal_writestring("desktop_init: Exiting with error -1\n");
        return -1; // Window manager init failed
    }
    terminal_writestring("desktop_init: Window manager initialized successfully\n");

    // Get framebuffer info
    terminal_writestring("desktop_init: Getting framebuffer info...\n");
    fb_info_t info;
    fb_get_info(&info);
    terminal_writestring("desktop_init: Framebuffer info retrieved\n");

    // UNCOMMENT THE FOLLOWING CODE - IT IS NEEDED FOR GUI SETUP:

    char buf[64];
    sprintf(buf, "desktop_init: Screen resolution: %dx%d with %d bits per pixel\n",
            info.width, info.height, info.bits_per_pixel);
    terminal_writestring(buf);

    // Set desktop theme
    terminal_writestring("desktop_init: Setting desktop theme\n");
    desktop_set_theme(THEME_TEAL);
    terminal_writestring("desktop_init: Desktop theme set\n");

    // Add desktop icons - adjusting for smaller screen
    terminal_writestring("desktop_init: Adding desktop icons\n");
    if (info.width <= 320) {
        // Use smaller, more compact icons for VGA mode
        desktop_add_icon("Files", 10, 10, FB_COLOR_BLUE, desktop_open_file_browser);
        desktop_add_icon("Term", 70, 10, FB_COLOR_GREEN, desktop_open_terminal);
        desktop_add_icon("Edit", 130, 10, FB_COLOR_RED, desktop_open_text_editor);
        desktop_add_icon("Set", 190, 10, FB_COLOR_PURPLE, desktop_open_settings);
    } else {
        // Standard icon placement for higher resolutions
        desktop_add_icon("Files", ICON_MARGIN_X, ICON_MARGIN_Y, FB_COLOR_BLUE, desktop_open_file_browser);
        desktop_add_icon("Terminal", ICON_MARGIN_X + ICON_SPACING_X, ICON_MARGIN_Y, FB_COLOR_GREEN, desktop_open_terminal);
        desktop_add_icon("Editor", ICON_MARGIN_X + ICON_SPACING_X * 2, ICON_MARGIN_Y, FB_COLOR_RED, desktop_open_text_editor);
        desktop_add_icon("Settings", ICON_MARGIN_X, ICON_MARGIN_Y + ICON_SPACING_Y, FB_COLOR_PURPLE, desktop_open_settings);
    }
    terminal_writestring("desktop_init: Desktop icons added\n");

    // Register mouse handler for desktop events
    terminal_writestring("desktop_init: Registering mouse handler\n");
    mouse_register_handler(desktop_mouse_handler);
    terminal_writestring("desktop_init: Mouse handler registered\n");

    terminal_writestring("desktop_init: Desktop environment initialized successfully\n");
    terminal_writestring("desktop_init: Exiting desktop_init() with success (0)\n");
    return 0; // Success
}

// Update the desktop display
void desktop_update(void) {
    static int first_update = 1;
    
    // Only print debug message on first update to reduce console spam
    if (first_update) {
        desktop_debug("Updating desktop display...");
        first_update = 0;
    }
    
    // Get framebuffer info
    fb_info_t info;
    fb_get_info(&info);
    
    // Clear screen with desktop background color to overwrite test pattern
    uint32_t bg_color = (info.bits_per_pixel == 8) ? 3 : desktop_bg_color; // Cyan for VGA
    uint8_t* buffer = info.buffer;
    
    if (!buffer) {
        desktop_debug("Error! Framebuffer buffer is NULL!");
        return;
    }
    
    if (info.bits_per_pixel == 8) {
        // For 8-bit VGA mode, use minimal debug to avoid console flood
        static int full_clearing_done = 0;
        
        if (!full_clearing_done) {
            desktop_debug("Performing first full screen clear...");
            for (uint32_t y = 0; y < info.height; y++) {
                for (uint32_t x = 0; x < info.width; x++) {
                    buffer[y * info.width + x] = bg_color;
                }
            }
            
            // Add a debug pixel to verify writing
            buffer[10 * info.width + 10] = 15; // White pixel at (10, 10)
            full_clearing_done = 1;
            desktop_debug("First screen clearing completed");
        }
    } else {
        fb_clear_screen(bg_color);
    }
    
    // Draw desktop elements
    draw_desktop();
    
    // Update window manager
    wm_update();
}

// Process desktop events
void desktop_process_events(void) {
    // Process window manager events
    wm_process_events();
    
    // Update taskbar clock
    update_clock();
}

// In desktop.c - update the desktop_run function
int desktop_run(void) {
    terminal_writestring("desktop_run: Starting desktop environment...\n");

    
    // Simple delay to reduce CPU usage - increased for better rendering
    for (volatile int i = 0; i < 2000000; i++) {
        // Empty delay loop
    }

    // Draw initial desktop
    terminal_writestring("desktop_run: Drawing initial desktop\n");
    desktop_update();

    // Get framebuffer info to determine screen size
    fb_info_t info;
    fb_get_info(&info);

    // Show welcome window - adjust size based on resolution
    terminal_writestring("desktop_run: Creating welcome window\n");
    uint32_t welcome_width = (info.width <= 320) ? 200 : 340;
    uint32_t welcome_height = (info.width <= 320) ? 100 : 200;
    uint32_t welcome_x = (info.width - welcome_width) / 2;
    uint32_t welcome_y = (info.height - welcome_height) / 3;

    window_t* welcome = window_create("Welcome to Hextrix", welcome_x, welcome_y, 
                                     welcome_width, welcome_height, WINDOW_STYLE_NORMAL);
    if (welcome) {
        terminal_writestring("desktop_run: Welcome window created\n");
        window_fill_rect(welcome, 0, 0, welcome->client_width, welcome->client_height, FB_COLOR_WHITE);
        window_draw_text(welcome, 10, 10, "Welcome to Hextrix OS", FB_COLOR_BLACK);
        window_draw_text(welcome, 10, 30, "VGA Mode: 320x200", FB_COLOR_BLACK);
        
        uint32_t btn_width = (info.width <= 320) ? 60 : 100;
        uint32_t btn_x = (welcome->client_width - btn_width) / 2;
        uint32_t btn_y = welcome->client_height - 40;
        
        control_create_button(welcome, 1, "Start", btn_x, btn_y, btn_width, 30);
        window_set_event_handler(welcome, welcome_window_handler);
        window_show(welcome);
        terminal_writestring("desktop_run: Welcome window shown\n");
    } else {
        terminal_writestring("desktop_run: Failed to create welcome window!\n");
    }

    // Update the screen once more to ensure welcome window is visible
    desktop_update();
    terminal_writestring("desktop_run: Entering main event loop\n");

    // Main desktop event loop
    uint32_t loop_count = 0;
    while (1) {
        // Process desktop events (mouse, keyboard, etc.)
        desktop_process_events();
        
        // Update desktop display
        desktop_update();
        
        // Simple delay to reduce CPU usage
        for (volatile int i = 0; i < 100000; i++) {
            // Empty delay loop
        }
        
        // Log every 1000 iterations
        loop_count++;
        if (loop_count % 1000 == 0) {
            char buf[32];
            sprintf(buf, "Desktop event loop: %u", loop_count);
            terminal_writestring(buf);
            terminal_writestring("\n");
        }
    }
    
    return -1; // Should never reach here
}

// Add an icon to the desktop
int desktop_add_icon(const char* name, uint32_t x, uint32_t y, uint32_t color, void (*action)(void)) {
    if (icon_count >= MAX_DESKTOP_ICONS) {
        terminal_writestring("Desktop: Too many icons\n");
        return -1;
    }
    
    // Initialize icon
    desktop_icon_t* icon = &desktop_icons[icon_count];
    strncpy(icon->name, name, sizeof(icon->name) - 1);
    icon->name[sizeof(icon->name) - 1] = '\0';
    icon->x = x;
    icon->y = y;
    icon->width = ICON_WIDTH;
    icon->height = ICON_HEIGHT;
    icon->icon_color = color;
    icon->action = action;
    
    icon_count++;
    
    // Log the icon addition
    char debug_msg[64];
    sprintf(debug_msg, "Added desktop icon '%s' at %d,%d", name, x, y);
    terminal_writestring(debug_msg);
    terminal_writestring("\n");
    
    return 0;
}

// Update background color based on theme
void desktop_set_theme(uint32_t theme) {
    current_theme = theme;
    
    switch (theme) {
        case THEME_BLUE:
            desktop_bg_color = FB_COLOR_BLUE;
            break;
        case THEME_GREEN:
            desktop_bg_color = FB_COLOR_GREEN;
            break;
        case THEME_TEAL:
            desktop_bg_color = FB_COLOR_CYAN;  // Use predefined cyan instead of teal
            break;
        default:
            desktop_bg_color = FB_COLOR_BLUE;  // Default to blue
            break;
    }
    
    // Log theme change
    char theme_name[16] = "Unknown";
    switch (theme) {
        case THEME_BLUE:  strcpy(theme_name, "Blue"); break;
        case THEME_GREEN: strcpy(theme_name, "Green"); break;
        case THEME_TEAL:  strcpy(theme_name, "Cyan"); break;
    }
    
    char debug_msg[64];
    sprintf(debug_msg, "Desktop theme set to %s", theme_name);
    terminal_writestring(debug_msg);
    terminal_writestring("\n");
}

// Draw the desktop background and elements
static void draw_desktop(void) {
    // The desktop background is already cleared with desktop_bg_color in desktop_update
    
    // Draw desktop icons
    draw_icons();
    
    // Draw taskbar
    draw_taskbar();
}

// Draw desktop icons
// Draw desktop icons with improved appearance
static void draw_icons(void) {
    // Get framebuffer info for scaling
    fb_info_t info;
    fb_get_info(&info);
    
    // Scale down icon size for low resolution
    uint32_t actual_icon_width = (info.width <= 320) ? 30 : ICON_WIDTH;
    uint32_t actual_icon_height = (info.height <= 200) ? 30 : ICON_HEIGHT;
    
    for (uint32_t i = 0; i < icon_count; i++) {
        desktop_icon_t* icon = &desktop_icons[i];
        
        // Draw icon background with 3D effect
        // Main icon body
        fb_draw_rectangle(icon->x, icon->y, actual_icon_width, actual_icon_height, icon->icon_color, 1);
        
        // Highlight (top and left edges)
        fb_draw_line(icon->x, icon->y, icon->x + actual_icon_width - 1, icon->y, FB_COLOR_WHITE);
        fb_draw_line(icon->x, icon->y, icon->x, icon->y + actual_icon_height - 1, FB_COLOR_WHITE);
        
        // Shadow (bottom and right edges)
        fb_draw_line(icon->x, icon->y + actual_icon_height - 1, 
                    icon->x + actual_icon_width - 1, icon->y + actual_icon_height - 1, FB_COLOR_DARK_GRAY);
        fb_draw_line(icon->x + actual_icon_width - 1, icon->y,
                    icon->x + actual_icon_width - 1, icon->y + actual_icon_height - 1, FB_COLOR_DARK_GRAY);
        
        // Draw a simple icon symbol
        switch (i) {
            case 0: // Files
                // Draw folder shape
                fb_draw_rectangle(icon->x + 5, icon->y + 8, actual_icon_width - 10, 4, FB_COLOR_WHITE, 1);
                fb_draw_rectangle(icon->x + 5, icon->y + 12, actual_icon_width - 10, actual_icon_height - 17, FB_COLOR_WHITE, 1);
                break;
            case 1: // Terminal
                // Draw terminal shape
                fb_draw_rectangle(icon->x + 5, icon->y + 5, actual_icon_width - 10, actual_icon_height - 10, FB_COLOR_BLACK, 1);
                fb_draw_rectangle(icon->x + 7, icon->y + 7, actual_icon_width - 14, 2, FB_COLOR_WHITE, 1);
                break;
            case 2: // Editor
                // Draw document shape
                fb_draw_rectangle(icon->x + 5, icon->y + 5, actual_icon_width - 10, actual_icon_height - 10, FB_COLOR_WHITE, 1);
                fb_draw_line(icon->x + 8, icon->y + 10, icon->x + actual_icon_width - 8, icon->y + 10, FB_COLOR_BLACK);
                fb_draw_line(icon->x + 8, icon->y + 13, icon->x + actual_icon_width - 8, icon->y + 13, FB_COLOR_BLACK);
                fb_draw_line(icon->x + 8, icon->y + 16, icon->x + actual_icon_width - 14, icon->y + 16, FB_COLOR_BLACK);
                break;
            case 3: // Settings
                // Draw gear shape
                fb_draw_circle(icon->x + actual_icon_width/2, icon->y + actual_icon_height/2, 
                             actual_icon_width/3, FB_COLOR_WHITE, 0);
                fb_draw_circle(icon->x + actual_icon_width/2, icon->y + actual_icon_height/2, 
                             actual_icon_width/6, FB_COLOR_WHITE, 1);
                break;
        }
        
        // Draw icon name with shadow for better visibility
        uint32_t text_width = strlen(icon->name) * 8;  // Assuming 8 pixel wide font
        uint32_t text_x = icon->x + (actual_icon_width - text_width) / 2;
        uint32_t text_y = icon->y + actual_icon_height + 5;
        
        // Draw text shadow for better visibility
        fb_draw_text(text_x + 1, text_y + 1, icon->name, FB_COLOR_BLACK);
        fb_draw_text(text_x, text_y, icon->name, FB_COLOR_WHITE);
    }
}
// Draw mouse cursor at specified position
static void draw_mouse_cursor(int16_t x, int16_t y) {
    if (!mouse_visible) return;
    
    fb_info_t info;
    fb_get_info(&info);
    
    // Ensure cursor stays within screen bounds
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + CURSOR_WIDTH >= info.width) x = info.width - CURSOR_WIDTH - 1;
    if (y + CURSOR_HEIGHT >= info.height) y = info.height - CURSOR_HEIGHT - 1;
    
    // First save the background
    uint8_t* buffer = info.buffer;
    int index = 0;
    
    for (int cy = 0; cy < CURSOR_HEIGHT; cy++) {
        for (int cx = 0; cx < CURSOR_WIDTH; cx++) {
            uint32_t screen_x = x + cx;
            uint32_t screen_y = y + cy;
            
            if (screen_x < info.width && screen_y < info.height) {
                if (info.bits_per_pixel == 8) {
                    uint32_t offset = screen_y * info.width + screen_x;
                    mouse_background[index++] = buffer[offset];
                } else if (info.bits_per_pixel == 32) {
                    uint32_t offset = screen_y * info.pitch + screen_x * 4;
                    mouse_background[index++] = *(uint32_t*)(buffer + offset);
                }
            }
        }
    }
    
    // Now draw the cursor
    for (int cy = 0; cy < CURSOR_HEIGHT; cy++) {
        uint8_t line = cursor_data[cy];
        
        for (int cx = 0; cx < CURSOR_WIDTH; cx++) {
            if (line & (0x80 >> cx)) { // Check if pixel is set in cursor pattern
                uint32_t screen_x = x + cx;
                uint32_t screen_y = y + cy;
                
                if (screen_x < info.width && screen_y < info.height) {
                    // Draw white pixel for cursor
                    fb_draw_pixel(screen_x, screen_y, FB_COLOR_WHITE);
                }
            }
        }
    }
    
    // Update last known position
    last_mouse_x = x;
    last_mouse_y = y;
}
// Restore background where mouse cursor was
static void restore_mouse_background() {
    if (last_mouse_x < 0 || last_mouse_y < 0) return;
    
    fb_info_t info;
    fb_get_info(&info);
    uint8_t* buffer = info.buffer;
    int index = 0;
    
    for (int cy = 0; cy < CURSOR_HEIGHT; cy++) {
        for (int cx = 0; cx < CURSOR_WIDTH; cx++) {
            uint32_t screen_x = last_mouse_x + cx;
            uint32_t screen_y = last_mouse_y + cy;
            
            if (screen_x < info.width && screen_y < info.height) {
                if (info.bits_per_pixel == 8) {
                    uint32_t offset = screen_y * info.width + screen_x;
                    buffer[offset] = mouse_background[index++];
                } else if (info.bits_per_pixel == 32) {
                    uint32_t offset = screen_y * info.pitch + screen_x * 4;
                    *(uint32_t*)(buffer + offset) = mouse_background[index++];
                }
            }
        }
    }
    
    last_mouse_x = -1;
    last_mouse_y = -1;
}
// Draw the taskbar with improved appearance
static void draw_taskbar(void) {
    fb_info_t info;
    fb_get_info(&info);
    
    // Draw taskbar background with gradient effect (if possible in 8-bit mode)
    uint32_t taskbar_bg_color = FB_COLOR_BLUE;
    uint32_t taskbar_highlight = FB_COLOR_GREEN;
    
    // Main taskbar background
    fb_draw_rectangle(0, info.height - taskbar.height, info.width, taskbar.height, taskbar_bg_color, 1);
    
    // Top highlight line
    fb_draw_line(0, info.height - taskbar.height, info.width, info.height - taskbar.height, taskbar_highlight);
    
    // Adjust for VGA mode
    uint32_t start_btn_width = (info.width <= 320) ? 50 : 80;
    uint32_t text_y_offset = (info.height <= 200) ? 3 : 10;
    
    // Draw start button with 3D effect
    fb_draw_rectangle(5, info.height - taskbar.height + 2, start_btn_width, 
                     taskbar.height - 4, FB_COLOR_GREEN, 1);
                     
    // Button highlights
    fb_draw_line(5, info.height - taskbar.height + 2, 
                5 + start_btn_width - 1, info.height - taskbar.height + 2, FB_COLOR_WHITE);
    fb_draw_line(5, info.height - taskbar.height + 2, 
                5, info.height - taskbar.height + taskbar.height - 3, FB_COLOR_WHITE);
                
    // Button shadows
    fb_draw_line(5, info.height - taskbar.height + taskbar.height - 3, 
                5 + start_btn_width - 1, info.height - taskbar.height + taskbar.height - 3, FB_COLOR_DARK_GRAY);
    fb_draw_line(5 + start_btn_width - 1, info.height - taskbar.height + 2, 
                5 + start_btn_width - 1, info.height - taskbar.height + taskbar.height - 3, FB_COLOR_DARK_GRAY);
    
    // Draw button text with shadow for better visibility              
    fb_draw_text(14, info.height - taskbar.height + text_y_offset + 1, "Start", FB_COLOR_BLACK);
    fb_draw_text(13, info.height - taskbar.height + text_y_offset, "Start", FB_COLOR_WHITE);
    
    // Draw clock if space allows
    if (info.width > 200) {
        // Update the clock (in case it hasn't been updated yet)
        update_clock();
        
        // Draw clock background
        fb_draw_rectangle(info.width - 65, info.height - taskbar.height + 2, 
                         60, taskbar.height - 4, FB_COLOR_BLUE, 1);
        
        // Draw clock text with shadow for better visibility
        fb_draw_text(info.width - 59, info.height - taskbar.height + text_y_offset + 1, 
                    taskbar.clock_text, FB_COLOR_BLACK);
        fb_draw_text(info.width - 60, info.height - taskbar.height + text_y_offset, 
                    taskbar.clock_text, FB_COLOR_WHITE);
    }
}
// Update the taskbar clock
static void update_clock(void) {
    // Get current time from system timer
    uint32_t current_time = hal_timer_get_ticks() / 100; // Convert to seconds
    
    // Calculate hours and minutes
    uint32_t seconds = current_time % 60;
    uint32_t minutes = (current_time / 60) % 60;
    uint32_t hours = (current_time / 3600) % 24;
    
    // Only update if changed
    static uint32_t last_update = 0xFFFFFFFF;
    if (seconds != last_update) {
        last_update = seconds;
        sprintf(taskbar.clock_text, "%02d:%02d", hours, minutes);
    }
}

// Mouse event handler for desktop
// Update the desktop_mouse_handler function in desktop.c:
// Update the desktop_mouse_handler function in desktop.c:
static void desktop_mouse_handler(mouse_event_t* event) {
    if (!event) return;
    
    // First restore the background where the cursor was
    restore_mouse_background();
    
    // Get framebuffer info
    fb_info_t info;
    fb_get_info(&info);
    
    // Draw the mouse cursor at new position
    draw_mouse_cursor(event->x, event->y);
    
    // Handle button press on desktop
    if ((event->buttons & MOUSE_BUTTON_LEFT) && !(event->prev_buttons & MOUSE_BUTTON_LEFT)) {
        // Check if clicked on taskbar
        if (event->y >= info.height - taskbar.height) {
            // Start button click
            uint32_t start_btn_width = (info.width <= 320) ? 50 : 80;
            if (event->x >= 5 && event->x < 5 + start_btn_width && 
                event->y >= info.height - taskbar.height + 2 && 
                event->y < info.height - taskbar.height + taskbar.height - 2) {
                
                terminal_writestring("Start button clicked\n");
                // TODO: Show start menu
                return;
            }
            return;
        }
        
        // Check if clicked on a desktop icon
        for (uint32_t i = 0; i < icon_count; i++) {
            desktop_icon_t* icon = &desktop_icons[i];
            
            // Scale icon size for low resolution
            uint32_t actual_icon_width = (info.width <= 320) ? 30 : ICON_WIDTH;
            uint32_t actual_icon_height = (info.height <= 200) ? 30 : ICON_HEIGHT;
            
            // Check if click is within icon
            if (event->x >= icon->x && event->x < icon->x + actual_icon_width &&
                event->y >= icon->y && event->y < icon->y + actual_icon_height) {
                
                char msg[64];
                sprintf(msg, "Icon '%s' clicked", icon->name);
                terminal_writestring(msg);
                terminal_writestring("\n");
                
                // Execute icon action
                if (icon->action) {
                    icon->action();
                }
                
                return;  // Event handled
            }
        }
    }
}

// In desktop.c - welcome_window_handler:
static int welcome_window_handler(window_t* window, window_message_t* msg) {
    if (!window || !msg) return 0;
    
    switch (msg->type) {
        case WM_CREATE:
            // Draw welcome content when the window is created
            window_fill_rect(window, 0, 0, window->client_width, window->client_height, FB_COLOR_WHITE);
            
            // Use higher contrast colors for better visibility in 8-bit mode
            window_draw_text(window, 10, 10, "Welcome to Hextrix OS", FB_COLOR_BLACK);
            window_draw_text(window, 10, 30, "VGA Mode: 320x200", FB_COLOR_BLACK);
            window_draw_text(window, 10, 50, "Click Start to begin", FB_COLOR_BLACK);
            return 1;
            
        case WM_CLOSE:
            // Close window when X is clicked
            window_destroy(window);
            return 1;
            
        case WM_CONTROL:
            // Handle button clicks
            if (msg->param1 == 1) { // Start button
                window_destroy(window);
                return 1;
            }
            break;
            
        case WM_PAINT:
            // Redraw content
            window_fill_rect(window, 0, 0, window->client_width, window->client_height, FB_COLOR_WHITE);
            window_draw_text(window, 10, 10, "Welcome to Hextrix OS", FB_COLOR_BLACK);
            window_draw_text(window, 10, 30, "VGA Mode: 320x200", FB_COLOR_BLACK);
            window_draw_text(window, 10, 50, "Click Start to begin", FB_COLOR_BLACK);
            return 1;
    }
    
    return 0;
}

// Placeholder implementations of application windows

// Open the file browser window
void desktop_open_file_browser(void) {
    terminal_writestring("Opening file browser...\n");
    
    // Create a simple window if it doesn't exist
    if (!file_browser_window) {
        file_browser_window = window_create("File Browser", 50, 50, 220, 160, WINDOW_STYLE_NORMAL);
        if (file_browser_window) {
            // Fill with a simple message
            window_fill_rect(file_browser_window, 0, 0, 
                           file_browser_window->client_width, 
                           file_browser_window->client_height, 
                           FB_COLOR_WHITE);
            window_draw_text(file_browser_window, 10, 10, "File Browser", FB_COLOR_BLACK);
            window_draw_text(file_browser_window, 10, 30, "Not implemented yet", FB_COLOR_BLACK);
            
            // Add a close button
            control_create_button(file_browser_window, 1, "Close", 
                               file_browser_window->client_width/2 - 30, 
                               file_browser_window->client_height - 40, 
                               60, 30);
            
            // Set event handler
            window_set_event_handler(file_browser_window, file_browser_event_handler);
        }
    }
    
    // Show window
    if (file_browser_window) {
        window_show(file_browser_window);
    }
}

// Open the terminal window
void desktop_open_terminal(void) {
    terminal_writestring("Opening terminal...\n");
    
    // Create a simple window if it doesn't exist
    if (!terminal_window) {
        terminal_window = window_create("Terminal", 80, 60, 240, 180, WINDOW_STYLE_NORMAL);
        if (terminal_window) {
            // Fill with a simple message
            window_fill_rect(terminal_window, 0, 0, 
                           terminal_window->client_width, 
                           terminal_window->client_height, 
                           FB_COLOR_BLACK);
            window_draw_text(terminal_window, 10, 10, "Terminal", FB_COLOR_WHITE);
            window_draw_text(terminal_window, 10, 30, "Ready >", FB_COLOR_WHITE);
            
            // Add a close button
            control_create_button(terminal_window, 1, "Close", 
                               terminal_window->client_width/2 - 30, 
                               terminal_window->client_height - 40, 
                               60, 30);
            
            // Set event handler
            window_set_event_handler(terminal_window, terminal_event_handler);
        }
    }
    
    // Show window
    if (terminal_window) {
        window_show(terminal_window);
    }
}

// Open the text editor window
void desktop_open_text_editor(void) {
    terminal_writestring("Opening text editor...\n");
    
    // Create a simple window if it doesn't exist
    if (!text_editor_window) {
        text_editor_window = window_create("Text Editor", 100, 40, 200, 150, WINDOW_STYLE_NORMAL);
        if (text_editor_window) {
            // Fill with a simple message
            window_fill_rect(text_editor_window, 0, 0, 
                           text_editor_window->client_width, 
                           text_editor_window->client_height, 
                           FB_COLOR_WHITE);
            window_draw_text(text_editor_window, 10, 10, "Text Editor", FB_COLOR_BLACK);
            window_draw_text(text_editor_window, 10, 30, "Not implemented yet", FB_COLOR_BLACK);
            
            // Add a close button
            control_create_button(text_editor_window, 1, "Close", 
                               text_editor_window->client_width/2 - 30, 
                               text_editor_window->client_height - 40, 
                               60, 30);
            
            // Set event handler
            window_set_event_handler(text_editor_window, text_editor_event_handler);
        }
    }
    
    // Show window
    if (text_editor_window) {
        window_show(text_editor_window);
    }
}

// Open the settings window
void desktop_open_settings(void) {
    terminal_writestring("Opening settings...\n");
    
    // Create a simple window if it doesn't exist
    if (!settings_window) {
        settings_window = window_create("Settings", 120, 80, 180, 140, WINDOW_STYLE_NORMAL);
        if (settings_window) {
            // Fill with a simple message
            window_fill_rect(settings_window, 0, 0, 
                           settings_window->client_width, 
                           settings_window->client_height, 
                           FB_COLOR_WHITE);
            window_draw_text(settings_window, 10, 10, "Settings", FB_COLOR_BLACK);
            window_draw_text(settings_window, 10, 30, "Not implemented yet", FB_COLOR_BLACK);
            
            // Add a close button
            control_create_button(settings_window, 1, "Close", 
                               settings_window->client_width/2 - 30, 
                               settings_window->client_height - 40, 
                               60, 30);
            
            // Set event handler
            window_set_event_handler(settings_window, settings_event_handler);
        }
    }
    
    // Show window
    if (settings_window) {
        window_show(settings_window);
    }
}

// Basic event handlers for the application windows

// File browser event handler
static int file_browser_event_handler(window_t* window, window_message_t* msg) {
    if (!window || !msg) return 0;
    
    switch (msg->type) {
        case WM_CLOSE:
            window_hide(window);
            return 1;
            
        case WM_CONTROL:
            if (msg->param1 == 1) { // Close button
                window_hide(window);
                return 1;
            }
            break;
    }
    
    return 0;
}

// Terminal event handler
static int terminal_event_handler(window_t* window, window_message_t* msg) {
    if (!window || !msg) return 0;
    
    switch (msg->type) {
        case WM_CLOSE:
            window_hide(window);
            return 1;
            
        case WM_CONTROL:
            if (msg->param1 == 1) { // Close button
                window_hide(window);
                return 1;
            }
            break;
    }
    
    return 0;
}

// Text editor event handler
static int text_editor_event_handler(window_t* window, window_message_t* msg) {
    if (!window || !msg) return 0;
    
    switch (msg->type) {
        case WM_CLOSE:
            window_hide(window);
            return 1;
            
        case WM_CONTROL:
            if (msg->param1 == 1) { // Close button
                window_hide(window);
                return 1;
            }
            break;
    }
    
    return 0;
}

// Settings event handler
static int settings_event_handler(window_t* window, window_message_t* msg) {
    if (!window || !msg) return 0;
    
    switch (msg->type) {
        case WM_CLOSE:
            window_hide(window);
            return 1;
            
        case WM_CONTROL:
            if (msg->param1 == 1) { // Close button
                window_hide(window);
                return 1;
            }
            break;
    }
    
    return 0;
}