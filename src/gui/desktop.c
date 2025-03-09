// src/gui/desktop.c
#include "gui/desktop.h"
#include "gui/window.h"
#include "hal_framebuffer.h"
#include "hal_mouse.h"
#include "terminal.h"
#include "stdio.h"
#include "kmalloc.h"
#include "string.h"

// Maximum number of desktop icons
#define MAX_DESKTOP_ICONS 16

// Desktop icons
static desktop_icon_t desktop_icons[MAX_DESKTOP_ICONS];
static uint32_t icon_count = 0;

// Desktop taskbar
static desktop_taskbar_t taskbar;

// Desktop background color
static uint32_t desktop_bg_color = 0xFF006060;  // Teal

// Program windows
static window_t* file_browser_window = NULL;
static window_t* terminal_window = NULL;
static window_t* text_editor_window = NULL;
static window_t* settings_window = NULL;

// Forward declarations
static void draw_desktop(void);
static void draw_taskbar(void);
static void draw_icons(void);
static int desktop_mouse_handler(mouse_event_t* event);
static int file_browser_event_handler(window_t* window, window_message_t* msg);
static int terminal_event_handler(window_t* window, window_message_t* msg);
static int text_editor_event_handler(window_t* window, window_message_t* msg);
static int settings_event_handler(window_t* window, window_message_t* msg);

// Initialize the desktop environment
int desktop_init(void) {
    // Initialize window manager
    if (wm_init() != 0) {
        terminal_writestring("Failed to initialize window manager\n");
        return -1;
    }
    
    // Initialize taskbar
    taskbar.height = 30;
    taskbar.color = 0xFF333333;  // Dark gray
    taskbar.text_color = 0xFFFFFFFF;  // White
    strcpy(taskbar.clock_text, "00:00");  // Default time
    
    // Register mouse handler
    mouse_register_handler(desktop_mouse_handler);
    
    // Initialize desktop icons
    icon_count = 0;
    
    // Add default icons
    desktop_add_icon("Files", 20, 20, FB_COLOR_BLUE, desktop_open_file_browser);
    desktop_add_icon("Terminal", 20, 100, FB_COLOR_GREEN, desktop_open_terminal);
    desktop_add_icon("Editor", 20, 180, FB_COLOR_YELLOW, desktop_open_text_editor);
    desktop_add_icon("Settings", 20, 260, FB_COLOR_PURPLE, desktop_open_settings);
    
    terminal_writestring("Desktop environment initialized\n");
    return 0;
}

// Process desktop events
void desktop_process_events(void) {
    // Process window manager events
    wm_process_events();
    
    // Update taskbar clock (this would normally be time-based)
    static uint32_t tick_count = 0;
    tick_count++;
    
    if (tick_count % 100 == 0) {  // Update clock every 100 ticks
        // In a real implementation, we'd use a real-time clock
        // For now, just increment the minutes
        int minutes = ((taskbar.clock_text[3] - '0') * 10) + (taskbar.clock_text[4] - '0');
        int hours = ((taskbar.clock_text[0] - '0') * 10) + (taskbar.clock_text[1] - '0');
        
        minutes++;
        if (minutes >= 60) {
            minutes = 0;
            hours++;
            if (hours >= 24) {
                hours = 0;
            }
        }
        
        sprintf(taskbar.clock_text, "%02d:%02d", hours, minutes);
        draw_taskbar();
    }
}

// Update the desktop display
void desktop_update(void) {
    // Clear screen with desktop background
    fb_clear(desktop_bg_color);
    
    // Draw desktop elements
    draw_desktop();
    
    // Update window manager (draws all windows)
    wm_update();
}

// Add an icon to the desktop
int desktop_add_icon(const char* name, uint32_t x, uint32_t y, uint32_t color, void (*action)(void)) {
    if (icon_count >= MAX_DESKTOP_ICONS) {
        return -1;
    }
    
    // Initialize icon
    desktop_icon_t* icon = &desktop_icons[icon_count];
    strncpy(icon->name, name, sizeof(icon->name) - 1);
    icon->name[sizeof(icon->name) - 1] = '\0';
    icon->x = x;
    icon->y = y;
    icon->width = 50;
    icon->height = 50;
    icon->icon_color = color;
    icon->action = action;
    
    icon_count++;
    
    return 0;
}

// Open the file browser window
void desktop_open_file_browser(void) {
    // If window already exists, just activate it
    if (file_browser_window) {
        window_show(file_browser_window);
        window_activate(file_browser_window);
        return;
    }
    
    // Create file browser window
    file_browser_window = window_create("File Browser", 100, 100, 400, 300, WINDOW_STYLE_NORMAL);
    if (!file_browser_window) {
        return;
    }
    
    // Set event handler
    window_set_event_handler(file_browser_window, file_browser_event_handler);
    
    // Show window
    window_show(file_browser_window);
    
    // Add some controls (in a real implementation, this would be more sophisticated)
    control_create_label(file_browser_window, 1, "File Browser", 10, 10, 100, 20);
    control_create_textbox(file_browser_window, 2, "/", 10, 40, 350, 25);
    control_create_button(file_browser_window, 3, "Go", 370, 40, 30, 25);
    control_create_listbox(file_browser_window, 4, 10, 75, 380, 190);
}

// Open the terminal window
void desktop_open_terminal(void) {
    // If window already exists, just activate it
    if (terminal_window) {
        window_show(terminal_window);
        window_activate(terminal_window);
        return;
    }
    
    // Create terminal window
    terminal_window = window_create("Terminal", 150, 150, 500, 350, WINDOW_STYLE_NORMAL);
    if (!terminal_window) {
        return;
    }
    
    // Set event handler
    window_set_event_handler(terminal_window, terminal_event_handler);
    
    // Show window
    window_show(terminal_window);
    
    // Draw terminal contents
    window_fill_rect(terminal_window, 0, 0, terminal_window->client_width, terminal_window->client_height, FB_COLOR_BLACK);
    window_draw_text(terminal_window, 5, 5, "Hextrix OS Terminal", FB_COLOR_WHITE);
    window_draw_text(terminal_window, 5, 20, "Type 'help' for available commands", FB_COLOR_WHITE);
    window_draw_text(terminal_window, 5, 35, "> ", FB_COLOR_WHITE);
}

// Open the text editor window
void desktop_open_text_editor(void) {
    // If window already exists, just activate it
    if (text_editor_window) {
        window_show(text_editor_window);
        window_activate(text_editor_window);
        return;
    }
    
    // Create text editor window
    text_editor_window = window_create("Text Editor", 200, 200, 450, 300, WINDOW_STYLE_NORMAL);
    if (!text_editor_window) {
        return;
    }
    
    // Set event handler
    window_set_event_handler(text_editor_window, text_editor_event_handler);
    
    // Show window
    window_show(text_editor_window);
    
    // Create a large text area
    control_create_textbox(text_editor_window, 1, "", 10, 10, 
                        text_editor_window->client_width - 20, 
                        text_editor_window->client_height - 50);
    
    // Create buttons
    control_create_button(text_editor_window, 2, "Save", 10, 
                        text_editor_window->client_height - 30, 80, 25);
    
    control_create_button(text_editor_window, 3, "Open", 100, 
                        text_editor_window->client_height - 30, 80, 25);
}

// Open the settings window
void desktop_open_settings(void) {
    // If window already exists, just activate it
    if (settings_window) {
        window_show(settings_window);
        window_activate(settings_window);
        return;
    }
    
    // Create settings window
    settings_window = window_create("Settings", 250, 150, 350, 300, WINDOW_STYLE_NORMAL);
    if (!settings_window) {
        return;
    }
    
    // Set event handler
    window_set_event_handler(settings_window, settings_event_handler);
    
    // Show window
    window_show(settings_window);
    
    // Add some controls
    control_create_label(settings_window, 1, "Display Settings", 10, 10, 150, 20);
    control_create_checkbox(settings_window, 2, "Enable desktop effects", 10, 40, 200, 20, 0);
    control_create_label(settings_window, 3, "Color Theme:", 10, 70, 100, 20);
    
    // Radio buttons for theme
    control_create_radiobutton(settings_window, 4, "Blue", 30, 100, 80, 20, 0);
    control_create_radiobutton(settings_window, 5, "Green", 30, 130, 80, 20, 0);
    control_create_radiobutton(settings_window, 6, "Teal", 30, 160, 80, 20, 1);
    
    // Apply button
    control_create_button(settings_window, 7, "Apply", 250, 250, 80, 30);
}

// Display the system information dialog
void desktop_show_system_info(void) {
    // Create system info window
    window_t* info_window = window_create("System Information", 175, 175, 300, 200, WINDOW_STYLE_NORMAL);
    if (!info_window) {
        return;
    }
    
    // Show window
    window_show(info_window);
    
    // Add information labels
    control_create_label(info_window, 1, "Hextrix OS v0.3.9-beta", 10, 10, 280, 20);
    control_create_label(info_window, 2, "GUI and Window Manager v1.0", 10, 30, 280, 20);
    control_create_label(info_window, 3, "CPU: Virtual x86", 10, 50, 280, 20);
    control_create_label(info_window, 4, "Memory: 8 MB", 10, 70, 280, 20);
    control_create_label(info_window, 5, "Display: 640x480x32", 10, 90, 280, 20);
    control_create_label(info_window, 6, "Developers: Jared Edwards", 10, 110, 280, 20);
    
    // OK button
    control_create_button(info_window, 7, "OK", 110, 150, 80, 30);
}

// Launch the desktop environment
void desktop_run(void) {
    if (desktop_init() != 0) {
        terminal_writestring("Failed to initialize desktop environment\n");
        return;
    }
    
    // Main loop
    while (1) {
        // Process events
        desktop_process_events();
        
        // Update display
        desktop_update();
        
        // Small delay to reduce CPU usage
        for (volatile int i = 0; i < 1000; i++) {}
    }
}

// Internal functions

// Draw the desktop background and elements
static void draw_desktop(void) {
    // Draw desktop background (already cleared with bg_color)
    
    // Draw taskbar
    draw_taskbar();
    
    // Draw desktop icons
    draw_icons();
}

// Draw the taskbar
static void draw_taskbar(void) {
    fb_info_t info;
    fb_get_info(&info);
    
    // Draw taskbar background
    fb_fill_rect(0, info.height - taskbar.height, info.width, taskbar.height, taskbar.color);
    
    // Draw start button
    fb_fill_rect(5, info.height - taskbar.height + 5, 80, taskbar.height - 10, FB_COLOR_BLUE);
    fb_draw_text(15, info.height - taskbar.height + 10, "Start", FB_COLOR_WHITE);
    
    // Draw clock
    fb_draw_text(info.width - 60, info.height - taskbar.height + 10, 
                taskbar.clock_text, taskbar.text_color);
    
    // Draw window buttons for open windows
    int btn_x = 90;
    
    if (file_browser_window && file_browser_window->visible) {
        fb_fill_rect(btn_x, info.height - taskbar.height + 5, 120, taskbar.height - 10, 0xFF555555);
        fb_draw_text(btn_x + 5, info.height - taskbar.height + 10, "File Browser", taskbar.text_color);
        btn_x += 125;
    }
    
    if (terminal_window && terminal_window->visible) {
        fb_fill_rect(btn_x, info.height - taskbar.height + 5, 120, taskbar.height - 10, 0xFF555555);
        fb_draw_text(btn_x + 5, info.height - taskbar.height + 10, "Terminal", taskbar.text_color);
        btn_x += 125;
    }
    
    if (text_editor_window && text_editor_window->visible) {
        fb_fill_rect(btn_x, info.height - taskbar.height + 5, 120, taskbar.height - 10, 0xFF555555);
        fb_draw_text(btn_x + 5, info.height - taskbar.height + 10, "Text Editor", taskbar.text_color);
        btn_x += 125;
    }
    
    if (settings_window && settings_window->visible) {
        fb_fill_rect(btn_x, info.height - taskbar.height + 5, 120, taskbar.height - 10, 0xFF555555);
        fb_draw_text(btn_x + 5, info.height - taskbar.height + 10, "Settings", taskbar.text_color);
    }
}

// Draw desktop icons
static void draw_icons(void) {
    for (uint32_t i = 0; i < icon_count; i++) {
        desktop_icon_t* icon = &desktop_icons[i];
        
        // Draw icon background
        fb_fill_rect(icon->x, icon->y, icon->width, icon->height, icon->icon_color);
        fb_draw_rect(icon->x, icon->y, icon->width, icon->height, FB_COLOR_WHITE);
        
        // Draw icon name
        uint32_t text_width = strlen(icon->name) * 8;  // Assuming 8 pixel wide font
        uint32_t text_x = icon->x + (icon->width - text_width) / 2;
        uint32_t text_y = icon->y + icon->height + 5;
        
        fb_draw_text(text_x, text_y, icon->name, FB_COLOR_WHITE);
    }
}

// Handle mouse events
static int desktop_mouse_handler(mouse_event_t* event) {
    // Check if mouse clicked on an icon
    if ((event->buttons & MOUSE_BUTTON_LEFT) && !(event->prev_buttons & MOUSE_BUTTON_LEFT)) {
        for (uint32_t i = 0; i < icon_count; i++) {
            desktop_icon_t* icon = &desktop_icons[i];
            
            // Check if click is within icon
            if (event->x >= icon->x && event->x < icon->x + icon->width &&
                event->y >= icon->y && event->y < icon->y + icon->height) {
                
                // Execute icon action
                if (icon->action) {
                    icon->action();
                }
                
                return 1;  // Event handled
            }
        }
        
        // Check if clicked on taskbar
        fb_info_t info;
        fb_get_info(&info);
        
        if (event->y >= info.height - taskbar.height) {
            // Check if clicked on start button
            if (event->x >= 5 && event->x < 85 && 
                event->y >= info.height - taskbar.height + 5 && 
                event->y < info.height - taskbar.height + taskbar.height - 5) {
                
                // Show system info (simulating a start menu)
                desktop_show_system_info();
                return 1;
            }
            
            // Check window buttons
            int btn_x = 90;
            
            if (file_browser_window && file_browser_window->visible) {
                if (event->x >= btn_x && event->x < btn_x + 120 &&
                    event->y >= info.height - taskbar.height + 5 && 
                    event->y < info.height - taskbar.height + taskbar.height - 5) {
                    
                    window_activate(file_browser_window);
                    return 1;
                }
                btn_x += 125;
            }
            
            if (terminal_window && terminal_window->visible) {
                if (event->x >= btn_x && event->x < btn_x + 120 &&
                    event->y >= info.height - taskbar.height + 5 && 
                    event->y < info.height - taskbar.height + taskbar.height - 5) {
                    
                    window_activate(terminal_window);
                    return 1;
                }
                btn_x += 125;
            }
            
            if (text_editor_window && text_editor_window->visible) {
                if (event->x >= btn_x && event->x < btn_x + 120 &&
                    event->y >= info.height - taskbar.height + 5 && 
                    event->y < info.height - taskbar.height + taskbar.height - 5) {
                    
                    window_activate(text_editor_window);
                    return 1;
                }
                btn_x += 125;
            }
            
            if (settings_window && settings_window->visible) {
                if (event->x >= btn_x && event->x < btn_x + 120 &&
                    event->y >= info.height - taskbar.height + 5 && 
                    event->y < info.height - taskbar.height + taskbar.height - 5) {
                    
                    window_activate(settings_window);
                    return 1;
                }
            }
        }
    }
    
    // Pass event to window manager
    return 0;
}

// Window event handlers

// File browser window event handler
static int file_browser_event_handler(window_t* window, window_message_t* msg) {
    switch (msg->type) {
        case WM_CLOSE:
            // Hide the window instead of destroying it
            window_hide(window);
            return 1;
            
        case WM_PAINT:
            // Window should be automatically painted by the window manager
            return 1;
            
        case WM_MOUSEDOWN:
            // Check if a button was clicked
            if (msg->param1 & MOUSE_BUTTON_LEFT) {
                uint32_t x = msg->param1 >> 16;
                uint32_t y = msg->param2;
                
                // Check "Go" button
                if (x >= 370 && x < 400 && y >= 40 && y < 65) {
                    // Simulate navigating to a directory
                    window_fill_rect(window, 10, 75, 380, 190, FB_COLOR_WHITE);
                    window_draw_text(window, 15, 80, "Directory:", FB_COLOR_BLACK);
                    window_draw_text(window, 30, 100, "/", FB_COLOR_BLACK);
                    window_draw_text(window, 15, 120, "Files:", FB_COLOR_BLACK);
                    window_draw_text(window, 30, 140, "kernel.bin", FB_COLOR_BLACK);
                    window_draw_text(window, 30, 160, "boot.asm", FB_COLOR_BLACK);
                    window_draw_text(window, 30, 180, "hal.c", FB_COLOR_BLACK);
                    return 1;
                }
            }
            break;
    }
    
    return 0;
}

// Terminal window event handler
static int terminal_event_handler(window_t* window, window_message_t* msg) {
    switch (msg->type) {
        case WM_CLOSE:
            // Hide the window instead of destroying it
            window_hide(window);
            return 1;
            
        case WM_PAINT:
            // Window should be automatically painted by the window manager
            return 1;
            
        case WM_KEYDOWN:
            // Handle key input (simulated)
            {
                static int cursor_x = 15;
                static int cursor_y = 35;
                static char command[256] = {0};
                static int command_length = 0;
                
                char key = (char)msg->param1;
                
                if (key == '\n') {
                    // Process command
                    window_draw_text(window, 5, cursor_y + 15, "> ", FB_COLOR_WHITE);
                    cursor_y += 15;
                    cursor_x = 15;
                    command[0] = '\0';
                    command_length = 0;
                } else if (key == '\b') {
                    // Backspace
                    if (command_length > 0) {
                        command_length--;
                        command[command_length] = '\0';
                        cursor_x -= 8;
                        window_fill_rect(window, cursor_x, cursor_y, 8, 12, FB_COLOR_BLACK);
                    }
                } else {
                    // Add character
                    if (command_length < 255) {
                        command[command_length++] = key;
                        command[command_length] = '\0';
                        window_draw_text(window, cursor_x, cursor_y, &key, FB_COLOR_WHITE);
                        cursor_x += 8;
                    }
                }
                
                return 1;
            }
    }
    
    return 0;
}

// Text editor window event handler
static int text_editor_event_handler(window_t* window, window_message_t* msg) {
    switch (msg->type) {
        case WM_CLOSE:
            // Hide the window instead of destroying it
            window_hide(window);
            return 1;
            
        case WM_PAINT:
            // Window should be automatically painted by the window manager
            return 1;
            
        case WM_MOUSEDOWN:
            // Check if a button was clicked
            if (msg->param1 & MOUSE_BUTTON_LEFT) {
                uint32_t x = msg->param1 >> 16;
                uint32_t y = msg->param2;
                
                // Check "Save" button
                if (x >= 10 && x < 90 && 
                    y >= window->client_height - 30 && 
                    y < window->client_height - 5) {
                    
                    // Simulate save operation
                    window_draw_text(window, 200, window->client_height - 20, "Saved!", FB_COLOR_GREEN);
                    return 1;
                }
                
                // Check "Open" button
                if (x >= 100 && x < 180 && 
                    y >= window->client_height - 30 && 
                    y < window->client_height - 5) {
                    
                    // Simulate open operation
                    window_draw_text(window, 200, window->client_height - 20, "Opened!", FB_COLOR_GREEN);
                    
                    // Simulate loading text into editor
                    window_fill_rect(window, 10, 10, 
                                   window->client_width - 20, 
                                   window->client_height - 50, 
                                   FB_COLOR_WHITE);
                    
                    window_draw_text(window, 15, 15, "# Hextrix OS Sample File", FB_COLOR_BLACK);
                    window_draw_text(window, 15, 30, "This is a sample text file for Hextrix OS.", FB_COLOR_BLACK);
                    window_draw_text(window, 15, 45, "The text editor is a basic component of the", FB_COLOR_BLACK);
                    window_draw_text(window, 15, 60, "graphical user interface.", FB_COLOR_BLACK);
                    
                    return 1;
                }
            }
            break;
    }
    
    return 0;
}

// Settings window event handler
static int settings_event_handler(window_t* window, window_message_t* msg) {
    switch (msg->type) {
        case WM_CLOSE:
            // Hide the window instead of destroying it
            window_hide(window);
            return 1;
            
        case WM_PAINT:
            // Window should be automatically painted by the window manager
            return 1;
            
        case WM_MOUSEDOWN:
            // Check if a button was clicked
            if (msg->param1 & MOUSE_BUTTON_LEFT) {
                uint32_t x = msg->param1 >> 16;
                uint32_t y = msg->param2;
                
                // Check "Apply" button
                if (x >= 250 && x < 330 && y >= 250 && y < 280) {
                    // Simulate applying settings
                    window_draw_text(window, 150, 250, "Applied!", FB_COLOR_GREEN);
                    
                    // Get selected theme (always Teal for the stub)
                    desktop_bg_color = 0xFF006060;  // Teal
                    
                    return 1;
                }
                
                // Check radio buttons
                if (x >= 30 && x < 110) {
                    // Blue theme
                    if (y >= 100 && y < 120) {
                        // Update radio buttons
                        window_fill_circle(window, 36, 106, 3, FB_COLOR_BLACK);
                        window_fill_circle(window, 36, 136, 3, FB_COLOR_WHITE);
                        window_fill_circle(window, 36, 166, 3, FB_COLOR_WHITE);
                        return 1;
                    }
                    
                    // Green theme
                    if (y >= 130 && y < 150) {
                        // Update radio buttons
                        window_fill_circle(window, 36, 106, 3, FB_COLOR_WHITE);
                        window_fill_circle(window, 36, 136, 3, FB_COLOR_BLACK);
                        window_fill_circle(window, 36, 166, 3, FB_COLOR_WHITE);
                        return 1;
                    }
                    
                    // Teal theme
                    if (y >= 160 && y < 180) {
                        // Update radio buttons
                        window_fill_circle(window, 36, 106, 3, FB_COLOR_WHITE);
                        window_fill_circle(window, 36, 136, 3, FB_COLOR_WHITE);
                        window_fill_circle(window, 36, 166, 3, FB_COLOR_BLACK);
                        return 1;
                    }
                }
            }
            break;
    }
    
    return 0;
}d normally be time-based)
    static uint32_t tick_count = 0;
    tick_count++;
    
    if (tick_count % 100 == 0) {  // Update clock every 100 ticks
        // In a real implementation, we'd use a real-time clock
        // For now, just increment the minutes
        int minutes = ((taskbar.clock_text[3] - '0') * 10) + (taskbar.clock_text[4] - '0');
        int hours = ((taskbar.clock_text[0] - '0') * 10) + (taskbar.clock_text[1] - '0');
        
        minutes++;
        if (minutes >= 60) {
            minutes = 0;
            hours++;
            if (hours >= 24) {
                hours = 0;
            }
        }
        
        sprintf(taskbar.clock_text, "%02d:%02d", hours, minutes);
        draw_taskbar();
    }
}

// Update the desktop display
void desktop_update(void) {
    // Clear screen with desktop background
    fb_clear(desktop_bg_color);
    
    // Draw desktop elements
    draw_desktop();
    
    // Update window manager (draws all windows)
    wm_update();
}

// Add an icon to the desktop
int desktop_add_icon(const char* name, uint32_t x, uint32_t y, uint32_t color, void (*action)(void)) {
    if (icon_count >= MAX_DESKTOP_ICONS) {
        return -1;
    }
    
    // Initialize icon
    desktop_icon_t* icon = &desktop_icons[icon_count];
    strncpy(icon->name, name, sizeof(icon->name) - 1);
    icon->name[sizeof(icon->name) - 1] = '\0';
    icon->x = x;
    icon->y = y;
    icon->width = 50;
    icon->height = 50;
    icon->icon_color = color;
    icon->action = action;
    
    icon_count++;
    
    return 0;
}

// Open the file browser window
void desktop_open_file_browser(void) {
    // If window already exists, just activate it
    if (file_browser_window) {
        window_show(file_browser_window);
        window_activate(file_browser_window);
        return;
    }
    
    // Create file browser window
    file_browser_window = window_create("File Browser", 100, 100, 400, 300, WINDOW_STYLE_NORMAL);
    if (!file_browser_window) {
        return;
    }
    
    // Set event handler
    window_set_event_handler(file_browser_window, file_browser_event_handler);
    
    // Show window
    window_show(file_browser_window);
    
    // Add some controls (in a real implementation, this would be more sophisticated)
    control_create_label(file_browser_window, 1, "File Browser", 10, 10, 100, 20);
    control_create_textbox(file_browser_window, 2, "/", 10, 40, 350, 25);
    control_create_button(file_browser_window, 3, "Go", 370, 40, 30, 25);
    control_create_listbox(file_browser_window, 4, 10, 75, 380, 190);
}

// Open the terminal window
void desktop_open_terminal(void) {
    // If window already exists, just activate it
    if (terminal_window) {
        window_show(terminal_window);
        window_activate(terminal_window);
        return;
    }
    
    // Create terminal window
    terminal_window = window_create("Terminal", 150, 150, 500, 350, WINDOW_STYLE_NORMAL);
    if (!terminal_window) {
        return;
    }
    
    // Set event handler
    window_set_event_handler(terminal_window, terminal_event_handler);
    
    // Show window
    window_show(terminal_window);
    
    // Draw terminal contents
    window_fill_rect(terminal_window, 0, 0, terminal_window->client_width, terminal_window->client_height, FB_COLOR_BLACK);
    window_draw_text(terminal_window, 5, 5, "Hextrix OS Terminal", FB_COLOR_WHITE);
    window_draw_text(terminal_window, 5, 20, "Type 'help' for available commands", FB_COLOR_WHITE);
    window_draw_text(terminal_window, 5, 35, "> ", FB_COLOR_WHITE);
}

// Open the text editor window
void desktop_open_text_editor(void) {
    // If window already exists, just activate it
    if (text_editor_window) {
        window_show(text_editor_window);
        window_activate(text_editor_window);
        return;
    }
    
    // Create text editor window
    text_editor_window = window_create("Text Editor", 200, 200, 450, 300, WINDOW_STYLE_NORMAL);
    if (!text_editor_window) {
        return;
    }
    
    // Set event handler
    window_set_event_handler(text_editor_window, text_editor_event_handler);
    
    // Show window
    window_show(text_editor_window);
    
    // Create a large text area
    control_create_textbox(text_editor_window, 1, "", 10, 10, 
                        text_editor_window->client_width - 20, 
                        text_editor_window->client_height - 50);
    
    // Create buttons
    control_create_button(text_editor_window, 2, "Save", 10, 
                        text_editor_window->client_height - 30, 80, 25);
    
    control_create_button(text_editor_window, 3, "Open", 100, 
                        text_editor_window->client_height - 30, 80, 25);
}

// Open the settings window
void desktop_open_settings(void) {
    // If window already exists, just activate it
    if (settings_window) {
        window_show(settings_window);
        window_activate(settings_window);
        return;
    }
    
    // Create settings window
    settings_window = window_create("Settings", 250, 150, 350, 300, WINDOW_STYLE_NORMAL);
    if (!settings_window) {
        return;
    }
    
    // Set event handler
    window_set_event_handler(settings_window, settings_event_handler);
    
    // Show window
    window_show(settings_window);
    
    // Add some controls
    control_create_label(settings_window, 1, "Display Settings", 10, 10, 150, 20);
    control_create_checkbox(settings_window, 2, "Enable desktop effects", 10, 40, 200, 20, 0);
    control_create_label(settings_window, 3, "Color Theme:", 10, 70, 100, 20);
    
    // Radio buttons for theme
    control_create_radiobutton(settings_window, 4, "Blue", 30, 100, 80, 20, 0);
    control_create_radiobutton(settings_window, 5, "Green", 30, 130, 80, 20, 0);
    control_create_radiobutton(settings_window, 6, "Teal", 30, 160, 80, 20, 1);
    
    // Apply button
    control_create_button(settings_window, 7, "Apply", 250, 250, 80, 30);
}

// Display the system information dialog
void desktop_show_system_info(void) {
    // Create system info window
    window_t* info_window = window_create("System Information", 175, 175