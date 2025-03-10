// src/gui/desktop.c
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
#define MAX_COMMAND_LENGTH 256
#define THEME_BLUE 0
#define THEME_GREEN 1
#define THEME_TEAL 2

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
static window_t* start_menu = NULL;
// Add this with other global window pointers
static window_t* active_window = NULL;  // Currently active window

// Desktop state
static uint8_t desktop_effects_enabled = 0;
static uint8_t current_theme = THEME_TEAL;

// Forward declarations
static void draw_desktop(void);
static void draw_taskbar(void);
static void draw_icons(void);
static void desktop_mouse_handler(mouse_event_t* event);
static int file_browser_event_handler(window_t* window, window_message_t* msg);
static int terminal_event_handler(window_t* window, window_message_t* msg);
static int text_editor_event_handler(window_t* window, window_message_t* msg);
static int settings_event_handler(window_t* window, window_message_t* msg);
static int info_window_event_handler(window_t* window, window_message_t* msg);
static int start_menu_event_handler(window_t* window, window_message_t* msg);
static int welcome_window_handler(window_t* window, window_message_t* msg);
static int message_window_handler(window_t* window, window_message_t* msg);
static void show_start_menu(uint32_t x, uint32_t y);
static void update_clock(void);
int desktop_init(void) {
    // Initialize window manager
    if (wm_init() != 0) {
        terminal_writestring("Failed to initialize window manager\n");
        return -1;
    }
    
    // Initialize taskbar
    taskbar.height = TASKBAR_HEIGHT;
    taskbar.color = 0xFF333333;  // Dark gray
    taskbar.text_color = 0xFFFFFFFF;  // White
    strcpy(taskbar.clock_text, "00:00");  // Default time
    
    // Register mouse handler
    mouse_register_handler(desktop_mouse_handler);
    
    // Initialize desktop icons
    icon_count = 0;
    
    // Add default icons
    desktop_add_icon("Files", ICON_MARGIN_X, ICON_MARGIN_Y, FB_COLOR_BLUE, desktop_open_file_browser);
    desktop_add_icon("Terminal", ICON_MARGIN_X, ICON_MARGIN_Y + ICON_SPACING_Y, FB_COLOR_GREEN, desktop_open_terminal);
    desktop_add_icon("Editor", ICON_MARGIN_X, ICON_MARGIN_Y + ICON_SPACING_Y * 2, FB_COLOR_YELLOW, desktop_open_text_editor);
    desktop_add_icon("Settings", ICON_MARGIN_X, ICON_MARGIN_Y + ICON_SPACING_Y * 3, FB_COLOR_PURPLE, desktop_open_settings);
    
    terminal_writestring("Desktop environment initialized\n");
    return 0;  // Return success
}

// Process desktop events
void desktop_process_events(void) {
    // Process window manager events
    wm_process_events();
    
    // Update taskbar clock
    update_clock();
}

// Update the desktop display
void desktop_update(void) {
    // Clear screen with desktop background
    fb_clear_screen(0x00000000);
    
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
    icon->width = ICON_WIDTH;
    icon->height = ICON_HEIGHT;
    icon->icon_color = color;
    icon->action = action;
    
    icon_count++;
    
    return 0;
}

// Update background color based on theme
void desktop_set_theme(uint32_t theme) {
    current_theme = theme;
    
    switch (theme) {
        case THEME_BLUE:
            desktop_bg_color = 0xFF0055AA;
            break;
        case THEME_GREEN:
            desktop_bg_color = 0xFF227722;
            break;
        case THEME_TEAL:
            desktop_bg_color = 0xFF006060;
            break;
        default:
            desktop_bg_color = 0xFF006060; // Default to teal
            break;
    }
}

int desktop_run(void) {
    if (desktop_init() != 0) {
        terminal_writestring("Failed to initialize desktop environment\n");
        return -1;
    }
    
    // Register the desktop mouse handler
    mouse_register_handler(desktop_mouse_handler);
    
    // Show a welcome window
    window_t* welcome = window_create("Welcome to Hextrix OS", 150, 100, 340, 200, WINDOW_STYLE_NORMAL);
    if (welcome) {
        window_fill_rect(welcome, 0, 0, welcome->client_width, welcome->client_height, FB_COLOR_WHITE);
        window_draw_text(welcome, 20, 20, "Welcome to Hextrix OS v0.4.0-beta", FB_COLOR_BLACK);
        window_draw_text(welcome, 20, 40, "This is the graphical user interface.", FB_COLOR_BLACK);
        window_draw_text(welcome, 20, 60, "Click on desktop icons to launch applications.", FB_COLOR_BLACK);
        window_draw_text(welcome, 20, 80, "Try the File Browser, Terminal, and Text Editor.", FB_COLOR_BLACK);
        window_draw_text(welcome, 20, 100, "Click the Start button for more options.", FB_COLOR_BLACK);
        
        control_create_button(welcome, 1, "Get Started", 120, 140, 100, 30);
        
        window_set_event_handler(welcome, welcome_window_handler);
        window_show(welcome);
    }
    
    // Main loop
    while (1) {
        // Process events
        desktop_process_events();
        
        // Update display
        desktop_update();
        
        // Small delay to reduce CPU usage
        hal_timer_delay(10); // Better than busy waiting
    }
    return 0;  // Unreachable, but included for completeness
}
// Draw the desktop background and elements
static void draw_desktop(void) {
    // Draw desktop background (already cleared with bg_color)
    
    // Draw desktop icons
    draw_icons();
    
    // Draw taskbar
    draw_taskbar();
}

static void draw_taskbar(void) {
    fb_info_t info;
    fb_get_info(&info);
    
    // Draw taskbar background
    fb_draw_rectangle(0, info.height - taskbar.height, info.width, taskbar.height, taskbar.color, 1);
    
    // Draw start button
    fb_draw_rectangle(5, info.height - taskbar.height + 5, TASKBAR_START_BUTTON_WIDTH, taskbar.height - 10, FB_COLOR_BLUE, 1);
    fb_draw_text(15, info.height - taskbar.height + 10, "Start", FB_COLOR_WHITE);
    
    // Draw clock
    fb_draw_text(info.width - 60, info.height - taskbar.height + 10, 
                taskbar.clock_text, taskbar.text_color);
    
    // Draw window buttons for open windows
    int btn_x = TASKBAR_START_BUTTON_WIDTH + 10;
    
    if (file_browser_window && file_browser_window->visible) {
        uint32_t btn_color = (file_browser_window == active_window) ? 0xFF777777 : 0xFF555555;
        fb_draw_rectangle(btn_x, info.height - taskbar.height + 5, TASKBAR_BUTTON_WIDTH, taskbar.height - 10, btn_color, 1);
        fb_draw_text(btn_x + 5, info.height - taskbar.height + 10, "File Browser", taskbar.text_color);
        btn_x += TASKBAR_BUTTON_WIDTH + 5;
    }
    
    if (terminal_window && terminal_window->visible) {
        uint32_t btn_color = (terminal_window == active_window) ? 0xFF777777 : 0xFF555555;
        fb_draw_rectangle(btn_x, info.height - taskbar.height + 5, TASKBAR_BUTTON_WIDTH, taskbar.height - 10, btn_color, 1);
        fb_draw_text(btn_x + 5, info.height - taskbar.height + 10, "Terminal", taskbar.text_color);
        btn_x += TASKBAR_BUTTON_WIDTH + 5;
    }
    
    if (text_editor_window && text_editor_window->visible) {
        uint32_t btn_color = (text_editor_window == active_window) ? 0xFF777777 : 0xFF555555;
        fb_draw_rectangle(btn_x, info.height - taskbar.height + 5, TASKBAR_BUTTON_WIDTH, taskbar.height - 10, btn_color, 1);
        fb_draw_text(btn_x + 5, info.height - taskbar.height + 10, "Text Editor", taskbar.text_color);
        btn_x += TASKBAR_BUTTON_WIDTH + 5;
    }
    
    if (settings_window && settings_window->visible) {
        uint32_t btn_color = (settings_window == active_window) ? 0xFF777777 : 0xFF555555;
        fb_draw_rectangle(btn_x, info.height - taskbar.height + 5, TASKBAR_BUTTON_WIDTH, taskbar.height - 10, btn_color, 1);
        fb_draw_text(btn_x + 5, info.height - taskbar.height + 10, "Settings", taskbar.text_color);
    }
}

static void draw_icons(void) {
    for (uint32_t i = 0; i < icon_count; i++) {
        desktop_icon_t* icon = &desktop_icons[i];
        
        // Draw icon background
        fb_draw_rectangle(icon->x, icon->y, icon->width, icon->height, icon->icon_color, 1);
        fb_draw_rectangle(icon->x, icon->y, icon->width, icon->height, FB_COLOR_WHITE, 0);
        
        // Draw icon name
        uint32_t text_width = strlen(icon->name) * 8;  // Assuming 8 pixel wide font
        uint32_t text_x = icon->x + (icon->width - text_width) / 2;
        uint32_t text_y = icon->y + icon->height + 5;
        
        // Draw text shadow for better visibility if effects are enabled
        if (desktop_effects_enabled) {
            fb_draw_text(text_x + 1, text_y + 1, icon->name, 0xFF000000);
        }
        
        fb_draw_text(text_x, text_y, icon->name, FB_COLOR_WHITE);
    }
}

// Update the taskbar clock
// Fixed update_clock function - using the correct seconds variable
static void update_clock(void) {
    // Get current time from system timer
    uint32_t current_time = hal_timer_get_ticks() / 100; // Convert to seconds
    
    // Calculate hours and minutes (seconds is actually used here)
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
    
    // Add file browser controls
    control_create_label(file_browser_window, 1, "File Browser", 10, 10, 100, 20);
    control_create_textbox(file_browser_window, 2, "/", 10, 40, 350, 25);
    control_create_button(file_browser_window, 3, "Go", 370, 40, 30, 25);
    
    // Create listbox for file display
    window_control_t* file_list = control_create_listbox(file_browser_window, 4, 
                                                        10, 75, 380, 190);
    
    // Add some example files
    control_listbox_add_item(file_list, "kernel.bin");
    control_listbox_add_item(file_list, "boot.asm");
    control_listbox_add_item(file_list, "hal.c");
    control_listbox_add_item(file_list, "window.c");
    control_listbox_add_item(file_list, "desktop.c");
    control_listbox_add_item(file_list, "terminal.c");
    control_listbox_add_item(file_list, "README.txt");
    
    // Show window
    window_show(file_browser_window);
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
    
    // Draw terminal contents
    window_fill_rect(terminal_window, 0, 0, terminal_window->client_width, terminal_window->client_height, FB_COLOR_BLACK);
    window_draw_text(terminal_window, 5, 5, "Hextrix OS Terminal", FB_COLOR_WHITE);
    window_draw_text(terminal_window, 5, 20, "Type 'help' for available commands", FB_COLOR_WHITE);
    window_draw_text(terminal_window, 5, 35, "> ", FB_COLOR_WHITE);
    
    // Store terminal state in user_data
    terminal_data_t* term_data = kmalloc(sizeof(terminal_data_t));
    if (term_data) {
        memset(term_data, 0, sizeof(terminal_data_t));
        term_data->cursor_x = 15;
        term_data->cursor_y = 35;
        term_data->command_length = 0;
        terminal_window->user_data = term_data;
    }
    
    // Show window
    window_show(terminal_window);
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
    
    // Create a large text area
    control_create_textbox(text_editor_window, 1, "", 10, 10, 
                        text_editor_window->client_width - 20, 
                        text_editor_window->client_height - 50);
    
    // Create buttons
    control_create_button(text_editor_window, 2, "Save", 10, 
                        text_editor_window->client_height - 30, 80, 25);
    
    control_create_button(text_editor_window, 3, "Open", 100, 
                        text_editor_window->client_height - 30, 80, 25);
    
    // Show window
    window_show(text_editor_window);
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
    
    // Add some controls
    control_create_label(settings_window, 1, "Display Settings", 10, 10, 150, 20);
    
    // Create checkbox for desktop effects
    window_control_t* effects_checkbox = control_create_checkbox(settings_window, 2, 
                                                              "Enable desktop effects", 
                                                              10, 40, 200, 20, 
                                                              desktop_effects_enabled);
    
    control_create_label(settings_window, 3, "Color Theme:", 10, 70, 100, 20);
    
    // Radio buttons for theme
    window_control_t* blue_radio = control_create_radiobutton(settings_window, 4, 
                                                           "Blue", 30, 100, 80, 20, 
                                                           (current_theme == THEME_BLUE));
    window_control_t* green_radio = control_create_radiobutton(settings_window, 5, 
                                                            "Green", 30, 130, 80, 20, 
                                                            (current_theme == THEME_GREEN));
    window_control_t* teal_radio = control_create_radiobutton(settings_window, 6, 
                                                           "Teal", 30, 160, 80, 20, 
                                                           (current_theme == THEME_TEAL));
    
    // Set the same group ID for all radio buttons
    blue_radio->group_id = 1;
    green_radio->group_id = 1;
    teal_radio->group_id = 1;
    
    // Apply button
    control_create_button(settings_window, 7, "Apply", 250, 250, 80, 30);
    
    // Show window
    window_show(settings_window);
}

// Display the system information dialog
void desktop_show_system_info(void) {
    // Create system info window
    window_t* info_window = window_create("System Information", 175, 175, 300, 200, WINDOW_STYLE_NORMAL);
    if (!info_window) {
        return;
    }
    
    // Add information labels
    control_create_label(info_window, 1, "Hextrix OS v0.4.0-beta", 10, 10, 280, 20);
    control_create_label(info_window, 2, "GUI and Window Manager v1.0", 10, 30, 280, 20);
    control_create_label(info_window, 3, "CPU: Virtual x86", 10, 50, 280, 20);
    control_create_label(info_window, 4, "Memory: 8 MB", 10, 70, 280, 20);
    control_create_label(info_window, 5, "Display: 640x480x32", 10, 90, 280, 20);
    control_create_label(info_window, 6, "© 2025 Jared Edwards", 10, 110, 280, 20);
    
    // OK button
    control_create_button(info_window, 7, "OK", 110, 150, 80, 30);
    
    // Define a custom event handler for the info window
    window_set_event_handler(info_window, info_window_event_handler);
    
    // Show window
    window_show(info_window);
}

// Helper function to create the desktop start menu
static void show_start_menu(uint32_t x, uint32_t y) {
    // If menu already exists, just destroy it (toggle behavior)
    if (start_menu) {
        window_destroy(start_menu);
        start_menu = NULL;
        return;
    }
    
    // Create start menu as a popup window
    start_menu = window_create("", x, y, 150, 180, WINDOW_STYLE_POPUP);
    if (!start_menu) {
        return;
    }
    
    // Fill background
    window_fill_rect(start_menu, 0, 0, 150, 180, 0xFF333333);
    window_draw_rect(start_menu, 0, 0, 150, 180, 0xFF555555);
    
    // Menu items (with blue highlight on hover)
    const char* menu_items[] = {
        "File Browser",
        "Terminal",
        "Text Editor",
        "Settings",
        "System Info",
        "Shutdown"
    };
    
    // Add menu items
    for (int i = 0; i < 6; i++) {
        window_fill_rect(start_menu, 5, 5 + i * 30, 140, 25, 0xFF333333);
        window_draw_text(start_menu, 10, 10 + i * 30, menu_items[i], 0xFFFFFFFF);
    }
    
    // Set up event handler for menu
    window_set_event_handler(start_menu, start_menu_event_handler);
    
    // Show menu
    window_show(start_menu);
    window_activate(start_menu);
}
// Main desktop mouse handler - handles clicks on desktop and taskbar
static void desktop_mouse_handler(mouse_event_t* event) {
    // Get screen dimensions
    fb_info_t info;
    fb_get_info(&info);
    
    // Check for desktop background clicks
    if ((event->buttons & MOUSE_BUTTON_LEFT) && !(event->prev_buttons & MOUSE_BUTTON_LEFT)) {
        // Check if clicked on taskbar
        if (event->y >= info.height - taskbar.height) {
            // Check if clicked on start button
            if (event->x >= 5 && event->x < 85 && 
                event->y >= info.height - taskbar.height + 5 && 
                event->y < info.height - taskbar.height + taskbar.height - 5) {
                
                // Show start menu
                show_start_menu(5, info.height - taskbar.height - 180);
                return;
            }
            
            // Check window buttons on taskbar
            int btn_x = TASKBAR_START_BUTTON_WIDTH + 10;
            
            if (file_browser_window && file_browser_window->visible) {
                if (event->x >= btn_x && event->x < btn_x + TASKBAR_BUTTON_WIDTH &&
                    event->y >= info.height - taskbar.height + 5 && 
                    event->y < info.height - taskbar.height + taskbar.height - 5) {
                    
                    window_activate(file_browser_window);
                    return;
                }
                btn_x += TASKBAR_BUTTON_WIDTH + 5;
            }
            
            if (terminal_window && terminal_window->visible) {
                if (event->x >= btn_x && event->x < btn_x + TASKBAR_BUTTON_WIDTH &&
                    event->y >= info.height - taskbar.height + 5 && 
                    event->y < info.height - taskbar.height + taskbar.height - 5) {
                    
                    window_activate(terminal_window);
                    return;
                }
                btn_x += TASKBAR_BUTTON_WIDTH + 5;
            }
            
            if (text_editor_window && text_editor_window->visible) {
                if (event->x >= btn_x && event->x < btn_x + TASKBAR_BUTTON_WIDTH &&
                    event->y >= info.height - taskbar.height + 5 && 
                    event->y < info.height - taskbar.height + taskbar.height - 5) {
                    
                    window_activate(text_editor_window);
                    return;
                }
                btn_x += TASKBAR_BUTTON_WIDTH + 5;
            }
            
            if (settings_window && settings_window->visible) {
                if (event->x >= btn_x && event->x < btn_x + TASKBAR_BUTTON_WIDTH &&
                    event->y >= info.height - taskbar.height + 5 && 
                    event->y < info.height - taskbar.height + taskbar.height - 5) {
                    
                    window_activate(settings_window);
                    return;
                }
            }
			return;
        }
        
        // Check if clicked on a desktop icon
        for (uint32_t i = 0; i < icon_count; i++) {
            desktop_icon_t* icon = &desktop_icons[i];
            
            // Check if click is within icon
            if (event->x >= icon->x && event->x < icon->x + icon->width &&
                event->y >= icon->y && event->y < icon->y + icon->height) {
                
                // Execute icon action
                if (icon->action) {
                    icon->action();
                }
                
                return;  // Event handled
            }
        }
    }
    
    // If we get here, event wasn't handled by desktop
    return;
}

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
            
        case WM_CONTROL:
            // Handle control events
            switch (msg->param1) { // Control ID
                case 3: // Go button
                    {
                        // Get the path from the textbox
                        window_control_t* path_textbox = NULL;
                        window_control_t* file_list = NULL;
                        
                        // Find controls
                        window_control_t* control = window->controls;
                        while (control) {
                            if (control->id == 2) {
                                path_textbox = control;
                            } else if (control->id == 4) {
                                file_list = control;
                            }
                            control = control->next;
                        }
                        
                        if (path_textbox && file_list) {
                            const char* path = control_get_text(path_textbox);
                            
                            // Clear the file list
                            control_listbox_clear(file_list);
                            
                            // Add some sample files based on the path
                            if (strcmp(path, "/") == 0) {
                                control_listbox_add_item(file_list, "bin/");
                                control_listbox_add_item(file_list, "boot/");
                                control_listbox_add_item(file_list, "dev/");
                                control_listbox_add_item(file_list, "etc/");
                                control_listbox_add_item(file_list, "home/");
                                control_listbox_add_item(file_list, "lib/");
                                control_listbox_add_item(file_list, "usr/");
                                control_listbox_add_item(file_list, "var/");
                                control_listbox_add_item(file_list, "kernel.bin");
                            } else if (strcmp(path, "/bin") == 0 || strcmp(path, "/bin/") == 0) {
                                control_listbox_add_item(file_list, "ls");
                                control_listbox_add_item(file_list, "cd");
                                control_listbox_add_item(file_list, "cp");
                                control_listbox_add_item(file_list, "mv");
                                control_listbox_add_item(file_list, "rm");
                                control_listbox_add_item(file_list, "mkdir");
                                control_listbox_add_item(file_list, "cat");
							} else if (strcmp(path, "/boot") == 0 || strcmp(path, "/boot/") == 0) {
                                control_listbox_add_item(file_list, "boot.asm");
                                control_listbox_add_item(file_list, "loader.bin");
                                control_listbox_add_item(file_list, "grub.cfg");
                                control_listbox_add_item(file_list, "initrd.img");
                            } else if (strcmp(path, "/home") == 0 || strcmp(path, "/home/") == 0) {
                                control_listbox_add_item(file_list, "user/");
                            } else if (strcmp(path, "/home/user") == 0 || strcmp(path, "/home/user/") == 0) {
                                control_listbox_add_item(file_list, "documents/");
                                control_listbox_add_item(file_list, "downloads/");
                                control_listbox_add_item(file_list, "photos/");
                                control_listbox_add_item(file_list, "README.txt");
                            } else {
                                // Default - no files found
                                control_listbox_add_item(file_list, "(No files found)");
                            }
                        }
                    }
                    return 1;
                
                case 4: // File list
                    {
                        // Handle double-click on file list item
                        window_control_t* file_list = NULL;
                        window_control_t* path_textbox = NULL;
                        
                        // Find controls
                        window_control_t* control = window->controls;
                        while (control) {
                            if (control->id == 2) {
                                path_textbox = control;
                            } else if (control->id == 4) {
                                file_list = control;
                            }
                            control = control->next;
                        }
                        
                        if (file_list && path_textbox) {
                            const char* selected = control_listbox_get_selected_text(file_list);
                            const char* current_path = control_get_text(path_textbox);
                            
                            if (selected && strlen(selected) > 0) {
                                // Check if it's a directory (ends with '/')
                                if (selected[strlen(selected) - 1] == '/') {
                                    char new_path[MAX_TEXTBOX_LENGTH];
                                    if (strcmp(current_path, "/") == 0) {
                                        snprintf(new_path, MAX_TEXTBOX_LENGTH, "/%s", selected);
                                    } else {
                                        snprintf(new_path, MAX_TEXTBOX_LENGTH, "%s%s", current_path, selected);
                                    }
                                    control_set_text(path_textbox, new_path);
                                    
                                    // Trigger "Go" button action
                                    window_send_message(window, WM_CONTROL, 3, 0);
                                }
                            }
                        }
                    }
                    return 1;
            }
            break;
    }
    
    return 0;
}

// Terminal window event handler
static int terminal_event_handler(window_t* window, window_message_t* msg) {
    // Get terminal data from user_data
    terminal_data_t* term_data = (terminal_data_t*)window->user_data;
    if (!term_data) return 0;
    
    switch (msg->type) {
        case WM_CLOSE:
            // Hide the window instead of destroying it
            window_hide(window);
            return 1;
            
        case WM_PAINT:
            // Window should be automatically painted by the window manager
            // But we need to draw the cursor
            if (term_data->cursor_visible) {
                window_draw_line(window, term_data->cursor_x, term_data->cursor_y, 
                               term_data->cursor_x, term_data->cursor_y + 12, FB_COLOR_WHITE);
            }
            return 1;
            
        case WM_KEYDOWN:
            // Handle key input
            {
                char key = (char)msg->param1;
                
                if (key == '\n') {
                    // Process command
                    term_data->command[term_data->command_length] = '\0';
                    
                    // Echo command
                    window_draw_text(window, 5, term_data->cursor_y + 15, "> ", FB_COLOR_WHITE);
                    
                    // Execute command
                    if (strcmp(term_data->command, "help") == 0) {
                        window_draw_text(window, 15, term_data->cursor_y + 15, "Available commands:", FB_COLOR_WHITE);
                        window_draw_text(window, 15, term_data->cursor_y + 30, "help - Show this help", FB_COLOR_WHITE);
                        window_draw_text(window, 15, term_data->cursor_y + 45, "clear - Clear screen", FB_COLOR_WHITE);
                        window_draw_text(window, 15, term_data->cursor_y + 60, "ls - List files", FB_COLOR_WHITE);
                        window_draw_text(window, 15, term_data->cursor_y + 75, "echo - Echo text", FB_COLOR_WHITE);
                        term_data->cursor_y += 90;
                    } else if (strcmp(term_data->command, "clear") == 0) {
                        window_fill_rect(window, 0, 0, window->client_width, window->client_height, FB_COLOR_BLACK);
                        term_data->cursor_y = 5;
                    } else if (strcmp(term_data->command, "ls") == 0) {
                        window_draw_text(window, 15, term_data->cursor_y + 15, "kernel.bin  boot.asm  hal.c  window.c", FB_COLOR_WHITE);
                        window_draw_text(window, 15, term_data->cursor_y + 30, "desktop.c  terminal.c  README.txt", FB_COLOR_WHITE);
                        term_data->cursor_y += 45;
                    } else if (strncmp(term_data->command, "echo ", 5) == 0) {
                        window_draw_text(window, 15, term_data->cursor_y + 15, term_data->command + 5, FB_COLOR_WHITE);
                        term_data->cursor_y += 30;
                    } else if (term_data->command_length > 0) {
                        window_draw_text(window, 15, term_data->cursor_y + 15, "Command not found: ", FB_COLOR_WHITE);
                        window_draw_text(window, 170, term_data->cursor_y + 15, term_data->command, FB_COLOR_WHITE);
                        term_data->cursor_y += 30;
                    } else {
                        term_data->cursor_y += 15;
                    }
                    
                    // Reset command
                    term_data->command_length = 0;
                    term_data->cursor_x = 15;
                    
                    // Draw new prompt
                    window_draw_text(window, 5, term_data->cursor_y, "> ", FB_COLOR_WHITE);
                    
                    // Handle scrolling if needed
                    if (term_data->cursor_y > window->client_height - 20) {
                        // Simple scrolling by clearing and repositioning
                        window_fill_rect(window, 0, 0, window->client_width, window->client_height, FB_COLOR_BLACK);
                        term_data->cursor_y = 5;
                        window_draw_text(window, 5, term_data->cursor_y, "> ", FB_COLOR_WHITE);
                    }
                    
                } else if (key == '\b') {
                    // Backspace
                    if (term_data->command_length > 0) {
                        term_data->command_length--;
                        term_data->cursor_x -= 8;
                        window_fill_rect(window, term_data->cursor_x, term_data->cursor_y, 8, 12, FB_COLOR_BLACK);
                    }
                } else if (key >= 32 && key <= 126) {
                    // Printable character
                    if (term_data->command_length < MAX_COMMAND_LENGTH - 1) {
                        term_data->command[term_data->command_length++] = key;
                        window_draw_text(window, term_data->cursor_x, term_data->cursor_y, &key, FB_COLOR_WHITE);
                        term_data->cursor_x += 8;
                    }
                }
                
                return 1;
            }
            break;
            
        case WM_TIMER:
            // Blink cursor
            term_data->cursor_visible = !term_data->cursor_visible;
            window_invalidate_region(window, term_data->cursor_x, term_data->cursor_y, 1, 12);
            return 1;
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
            
        case WM_CONTROL:
            // Handle control events
            switch (msg->param1) { // Control ID
                case 2: // Save button
                    {
                        // Find the status text area (or create it if it doesn't exist)
                        window_control_t* status_label = NULL;
                        window_control_t* control = window->controls;
                        
                        while (control) {
                            if (control->id == 4) {
                                status_label = control;
                                break;
                            }
                            control = control->next;
                        }
                        
                        if (!status_label) {
                            // Create status label
                            status_label = control_create_label(window, 4, "", 
                                                           200, window->client_height - 20, 
                                                           200, 20);
                        }
                        
                        // Update status text
                        control_set_text(status_label, "File saved successfully!");
                        
                        // Set up a timer to clear the status message
                        window_post_message(window, WM_TIMER, 1, 3000); // 3000ms = 3s
                    }
                    return 1;
                    
                case 3: // Open button
                    {
                        // Find the text area and status label
                        window_control_t* text_area = NULL;
                        window_control_t* status_label = NULL;
                        window_control_t* control = window->controls;
                        
                        while (control) {
                            if (control->id == 1) {
                                text_area = control;
                            } else if (control->id == 4) {
                                status_label = control;
                            }
                            control = control->next;
                        }
                        
                        if (!text_area) return 1;
                        
                        // Set some sample text
                        control_set_text(text_area, 
                                      "# Hextrix OS Sample File\n\n"
                                      "This is a sample text file for Hextrix OS.\n"
                                      "The text editor is a basic component of the\n"
                                      "graphical user interface.\n\n"
                                      "Features to implement:\n"
                                      "- File saving and loading\n"
                                      "- Text selection\n"
                                      "- Copy and paste\n"
                                      "- Syntax highlighting");
                        
                        // Create or update status label
                        if (!status_label) {
                            status_label = control_create_label(window, 4, "", 
                                                           200, window->client_height - 20, 
                                                           200, 20);
                        }
                        
                        // Set status text
                        control_set_text(status_label, "File opened successfully!");
                        
                        // Set up a timer to clear the status message
                        window_post_message(window, WM_TIMER, 1, 3000); // 3000ms = 3s
                    }
                    return 1;
            }
            break;
            
        case WM_TIMER:
            if (msg->param1 == 1) {
                // Clear status message
                window_control_t* status_label = NULL;
                window_control_t* control = window->controls;
                
                while (control) {
                    if (control->id == 4) {
                        status_label = control;
                        break;
                    }
                    control = control->next;
                }
                
                if (status_label) {
                    control_set_text(status_label, "");
                }
                
                return 1;
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
            
        case WM_CONTROL:
            // Handle control events
            switch (msg->param1) { // Control ID
                case 2: // Desktop effects checkbox
                    {
                        // Find the checkbox
                        window_control_t* effects_checkbox = NULL;
                        window_control_t* control = window->controls;
                        
                        while (control) {
                            if (control->id == 2) {
                                effects_checkbox = control;
                                break;
                            }
                            control = control->next;
                        }
                        
                        if (effects_checkbox) {
                            // Toggle effects state
                            desktop_effects_enabled = control_get_checked(effects_checkbox);
                        }
                    }
                    return 1;
                    
                case 4: // Blue theme radio
                case 5: // Green theme radio
                case 6: // Teal theme radio
                    // Store the theme selection (will be applied when Apply is clicked)
                    return 1;
                    
                case 7: // Apply button
                    {
                        // Find the status label or create it
                        window_control_t* status_label = NULL;
                        window_control_t* control = window->controls;
                        
                        while (control) {
                            if (control->id == 8) {
                                status_label = control;
                                break;
                            }
                            control = control->next;
                        }
                        
                        if (!status_label) {
                            status_label = control_create_label(window, 8, "", 
                                                             150, 250, 80, 20);
                        }
                        
                        // Find the radio buttons to determine the theme
                        window_control_t* blue_radio = NULL;
                        window_control_t* green_radio = NULL;
                        window_control_t* teal_radio = NULL;
                        
                        control = window->controls;
                        while (control) {
                            if (control->id == 4) {
                                blue_radio = control;
                            } else if (control->id == 5) {
                                green_radio = control;
                            } else if (control->id == 6) {
                                teal_radio = control;
                            }
                            control = control->next;
                        }
                        
                        // Determine which theme is selected
                        if (blue_radio && control_get_checked(blue_radio)) {
                            desktop_set_theme(THEME_BLUE);
                        } else if (green_radio && control_get_checked(green_radio)) {
                            desktop_set_theme(THEME_GREEN);
                        } else if (teal_radio && control_get_checked(teal_radio)) {
                            desktop_set_theme(THEME_TEAL);
                        }
                        
                        // Update status text
                        control_set_text(status_label, "Applied!");
                        
                        // Set up a timer to clear the status message
                        window_post_message(window, WM_TIMER, 1, 3000); // 3000ms = 3s
                    }
                    return 1;
            }
            break;
            
        case WM_TIMER:
            if (msg->param1 == 1) {
                // Clear status message
                window_control_t* status_label = NULL;
                window_control_t* control = window->controls;
                
                while (control) {
                    if (control->id == 8) {
                        status_label = control;
                        break;
                    }
                    control = control->next;
                }
                
                if (status_label) {
                    control_set_text(status_label, "");
                }
                
                return 1;
            }
            break;
    }
    
    return 0;
}

// Event handler for system info window
static int info_window_event_handler(window_t* window, window_message_t* msg) {
    switch (msg->type) {
        case WM_CLOSE:
            // Destroy the window (don't just hide it)
            window_destroy(window);
            return 1;
            
        case WM_CONTROL:
            if (msg->param1 == 7) { // OK button
                // Close the window
                window_destroy(window);
                return 1;
            }
            break;
    }
    
    return 0;
}

// Event handler for start menu
static int start_menu_event_handler(window_t* window, window_message_t* msg) {
    switch (msg->type) {
        case WM_CLOSE:
            window_destroy(window);
            start_menu = NULL;
            return 1;
            
        case WM_MOUSEMOVE:
            {
                uint32_t x = msg->param1;
                uint32_t y = msg->param2;
                
                // Highlight menu item under mouse
                for (int i = 0; i < 6; i++) {
                    uint32_t color = 0xFF333333; // Default color
                    
                    if (x >= 5 && x < 145 && y >= 5 + i * 30 && y < 30 + i * 30) {
                        color = 0xFF0055AA; // Highlight color
                    }
                    
                    window_fill_rect(window, 5, 5 + i * 30, 140, 25, color);
                    window_draw_text(window, 10, 10 + i * 30, 
                                   (i == 0) ? "File Browser" :
                                   (i == 1) ? "Terminal" :
                                   (i == 2) ? "Text Editor" :
                                   (i == 3) ? "Settings" :
                                   (i == 4) ? "System Info" : "Shutdown", 
                                   0xFFFFFFFF);
                }
                return 1;
            }
            
        case WM_MOUSEDOWN:
            if (msg->param1 & MOUSE_BUTTON_LEFT) {
                uint32_t x = msg->param1 >> 16;
                uint32_t y = msg->param2;
                
                // Check which menu item was clicked
                int item = -1;
                if (x >= 5 && x < 145) {
                    for (int i = 0; i < 6; i++) {
                        if (y >= 5 + i * 30 && y < 30 + i * 30) {
                            item = i;
                            break;
                        }
                    }
                }
                
                // Handle menu item click
                switch (item) {
                    case 0: // File Browser
                        desktop_open_file_browser();
                        break;
                    case 1: // Terminal
                        desktop_open_terminal();
                        break;
                    case 2: // Text Editor
                        desktop_open_text_editor();
                        break;
                    case 3: // Settings
                        desktop_open_settings();
                        break;
                    case 4: // System Info
                        desktop_show_system_info();
                        break;
                    case 5: // Shutdown (not implemented)
                        {
                            window_t* msg_win = window_create("System", 200, 200, 240, 120, WINDOW_STYLE_DIALOG);
                            if (msg_win) {
                                window_fill_rect(msg_win, 0, 0, msg_win->client_width, msg_win->client_height, FB_COLOR_WHITE);
                                window_draw_text(msg_win, 20, 20, "System cannot be shutdown", FB_COLOR_BLACK);
                                window_draw_text(msg_win, 20, 40, "in demonstration mode.", FB_COLOR_BLACK);
                                control_create_button(msg_win, 1, "OK", 80, 70, 80, 30);
                                window_set_event_handler(msg_win, message_window_handler);
                                window_show(msg_win);
                            }
                        }
                        break;
                }
                
                // Close the menu
                window_destroy(window);
                start_menu = NULL;
                return 1;
            }
            break;
            
        case WM_KILLFOCUS:
            // Close menu when focus is lost
            window_destroy(window);
            start_menu = NULL;
            return 1;
    }
    
    return 0;
}

// Event handler for welcome window
static int welcome_window_handler(window_t* window, window_message_t* msg) {
    switch (msg->type) {
        case WM_CLOSE:
            window_destroy(window);
            return 1;
            
        case WM_CONTROL:
            if (msg->param1 == 1) { // "Get Started" button
                // Close welcome window
                window_destroy(window);
                return 1;
            }
            break;
    }
    
    return 0;
}

// Event handler for message windows
static int message_window_handler(window_t* window, window_message_t* msg) {
    switch (msg->type) {
        case WM_CLOSE:
            window_destroy(window);
            return 1;
            
        case WM_CONTROL:
            if (msg->param1 == 1) { // OK button
                window_destroy(window);
                return 1;
            }
            break;
    }
    
    return 0;
}
