// include/gui/desktop.h
#ifndef GUI_DESKTOP_H
#define GUI_DESKTOP_H

#include <stdint.h>
#include "gui/window.h"

// Maximum command length for terminal
#define MAX_COMMAND_LENGTH 256

// Terminal data structure
typedef struct {
    char command[MAX_COMMAND_LENGTH];
    int command_length;
    int cursor_x;
    int cursor_y;
    uint8_t cursor_visible;
} terminal_data_t;

// Desktop icon structure
typedef struct {
    char name[32];        // Icon name
    uint32_t x;           // Icon X position
    uint32_t y;           // Icon Y position
    uint32_t width;       // Icon width
    uint32_t height;      // Icon height
    uint32_t icon_color;  // Icon color
    void (*action)(void); // Action to perform when clicked
} desktop_icon_t;

// Desktop taskbar structure
typedef struct {
    uint32_t height;      // Taskbar height
    uint32_t color;       // Taskbar background color
    uint32_t text_color;  // Taskbar text color
    char clock_text[16];  // Current time text
} desktop_taskbar_t;

// Initialize the desktop environment
int desktop_init(void);

// Process desktop events
void desktop_process_events(void);

// Update the desktop display
void desktop_update(void);

// Add an icon to the desktop
int desktop_add_icon(const char* name, uint32_t x, uint32_t y, uint32_t color, void (*action)(void));

// Set the desktop theme
void desktop_set_theme(uint32_t theme);

// Open the file browser window
void desktop_open_file_browser(void);

// Open the terminal window
void desktop_open_terminal(void);

// Open the text editor window
void desktop_open_text_editor(void);

// Open the settings window
void desktop_open_settings(void);

// Display the system information dialog
void desktop_show_system_info(void);

// Launch the desktop environment
void desktop_run(void);

#endif // GUI_DESKTOP_H