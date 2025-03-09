// include/gui/window.h
#ifndef GUI_WINDOW_H
#define GUI_WINDOW_H

#include <stdint.h>
#include "hal_framebuffer.h"
#include "hal_mouse.h"

// Window states
#define WINDOW_STATE_NORMAL    0
#define WINDOW_STATE_MINIMIZED 1
#define WINDOW_STATE_MAXIMIZED 2
#define WINDOW_STATE_HIDDEN    3

// Window styles
#define WINDOW_STYLE_NORMAL     0x0001  // Normal window with title bar and borders
#define WINDOW_STYLE_NOBORDER   0x0002  // Window without borders
#define WINDOW_STYLE_NOTITLE    0x0004  // Window without title bar
#define WINDOW_STYLE_NORESIZE   0x0008  // Window cannot be resized
#define WINDOW_STYLE_NOCLOSE    0x0010  // Window cannot be closed
#define WINDOW_STYLE_NOMINIMIZE 0x0020  // Window cannot be minimized
#define WINDOW_STYLE_NOMAXIMIZE 0x0040  // Window cannot be maximized
#define WINDOW_STYLE_DIALOG     0x0080  // Dialog window (modal)
#define WINDOW_STYLE_POPUP      0x0100  // Popup window (no title bar, temporary)
#define WINDOW_STYLE_FRAMELESS  (WINDOW_STYLE_NOBORDER | WINDOW_STYLE_NOTITLE)

// Window message types
#define WM_CREATE      0x0001
#define WM_DESTROY     0x0002
#define WM_PAINT       0x0003
#define WM_CLOSE       0x0004
#define WM_MOVE        0x0005
#define WM_SIZE        0x0006
#define WM_ACTIVATE    0x0007
#define WM_SETFOCUS    0x0008
#define WM_KILLFOCUS   0x0009
#define WM_MOUSEMOVE   0x000A
#define WM_MOUSEDOWN   0x000B
#define WM_MOUSEUP     0x000C
#define WM_MOUSEDRAG   0x000D
#define WM_MOUSEWHEEL  0x000E
#define WM_KEYDOWN     0x000F
#define WM_KEYUP       0x0010
#define WM_CHAR        0x0011
#define WM_COMMAND     0x0012
#define WM_CONTROL     0x0013  // Control-related message
#define WM_TIMER       0x0014  // Timer event
#define WM_USER        0x0400  // Base value for user-defined messages

// Maximum number of windows
#define MAX_WINDOWS 32

// Constants for control types
#define CONTROL_TYPE_BUTTON   1
#define CONTROL_TYPE_LABEL    2
#define CONTROL_TYPE_TEXTBOX  3
#define CONTROL_TYPE_CHECKBOX 4
#define CONTROL_TYPE_RADIO    5
#define CONTROL_TYPE_LISTBOX  6

// Constants for maximum lengths
#define MAX_WINDOW_TITLE      64
#define MAX_TEXTBOX_LENGTH    256
#define MAX_LISTBOX_ITEMS     100

// Forward declarations
struct window;
struct window_control;

// Window message structure
typedef struct {
    uint32_t type;             // Message type
    uint32_t param1;           // Parameter 1
    uint32_t param2;           // Parameter 2
    struct window* window;     // Target window
} window_message_t;

// Window event handler function type
typedef int (*window_event_handler_t)(struct window* window, window_message_t* msg);

// Control event handler function type
typedef int (*control_event_handler_t)(struct window_control* control, window_message_t* msg);

// Listbox data structure
typedef struct {
    char*    items[MAX_LISTBOX_ITEMS];  // Array of item texts
    uint32_t item_count;                // Number of items
    uint32_t first_visible_item;        // First visible item index
    uint32_t visible_items;             // Number of visible items
    int32_t  selected_index;            // Selected item index
} listbox_data_t;

// Window control structure
typedef struct window_control {
    struct window*         parent;           // Parent window
    uint32_t               id;               // Control ID
    uint32_t               x, y;             // Position relative to client area
    uint32_t               width, height;    // Size
    uint8_t                type;             // Control type
    uint8_t                enabled;          // Enabled flag
    uint8_t                visible;          // Visible flag
    uint8_t                focused;          // Focus flag
    uint32_t               group_id;         // Group ID (for radio buttons)
    uint32_t               style;            // Control style
    uint32_t               bg_color;         // Background color
    uint32_t               fg_color;         // Foreground color
    uint32_t               border_color;     // Border color
    void*                  control_data;     // Control-specific data
    void*                  control_data_extra; // Additional control data
    control_event_handler_t event_handler;   // Event handler
    struct window_control* next;             // Next control in list
} window_control_t;

// Window structure
typedef struct window {
    uint32_t               id;                 // Window ID
    char                   title[MAX_WINDOW_TITLE]; // Window title
    uint32_t               x, y;               // Position
    uint32_t               width, height;      // Size
    uint32_t               min_width, min_height; // Minimum size
    uint32_t               max_width, max_height; // Maximum size
    uint32_t               prev_x, prev_y;     // Previous position (for restore)
    uint32_t               prev_width, prev_height; // Previous size (for restore)
    uint32_t               client_x, client_y; // Client area position
    uint32_t               client_width, client_height; // Client area size
    uint32_t               style;              // Window style
    uint32_t               state;              // Window state
    uint8_t                visible;            // Visible flag
    uint8_t                active;             // Active flag
    uint8_t                dragging;           // Dragging flag
    uint8_t                resizing;           // Resizing flag
    uint8_t                drag_region;        // Resize region
    uint16_t               drag_offset_x;      // Drag offset X
    uint16_t               drag_offset_y;      // Drag offset Y
    int32_t                z_order;            // Z-order
    uint32_t               bg_color;           // Background color
    uint32_t               fg_color;           // Foreground color
    uint32_t               border_color;       // Border color
    uint32_t               title_bg_color;     // Title bar background color
    uint32_t               title_fg_color;     // Title bar foreground color
    void*                  buffer;             // Window buffer (if any)
    window_event_handler_t event_handler;      // Event handler
    window_control_t*      controls;           // Controls list
    window_control_t*      focused_control;    // Currently focused control
    
    // Parent/child relationship
    struct window* parent;          // Parent window
    struct window* first_child;     // First child window
    struct window* next_sibling;    // Next sibling window
    
    // User data (application-specific)
    void* user_data;
} window_t;

// Window manager functions
int wm_init(void);
void wm_process_events(void);
void wm_update(void);
void wm_invalidate_region(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void wm_update_region(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void wm_redraw_all(void);

// Window functions
window_t* window_create(const char* title, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t style);
void window_destroy(window_t* window);
void window_show(window_t* window);
void window_hide(window_t* window);
void window_move(window_t* window, uint32_t x, uint32_t y);
void window_resize(window_t* window, uint32_t width, uint32_t height);
void window_set_title(window_t* window, const char* title);
void window_set_style(window_t* window, uint32_t style);
void window_set_state(window_t* window, uint32_t state);
void window_activate(window_t* window);
void window_set_event_handler(window_t* window, window_event_handler_t handler);
void window_invalidate(window_t* window);
void window_invalidate_region(window_t* window, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
int window_send_message(window_t* window, uint32_t msg_type, uint32_t param1, uint32_t param2);
int window_post_message(window_t* window, uint32_t msg_type, uint32_t param1, uint32_t param2);

// Window drawing functions
void window_draw_pixel(window_t* window, uint32_t x, uint32_t y, uint32_t color);
void window_draw_line(window_t* window, uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t color);
void window_draw_rect(window_t* window, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void window_fill_rect(window_t* window, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void window_draw_circle(window_t* window, uint32_t x0, uint32_t y0, uint32_t radius, uint32_t color);
void window_fill_circle(window_t* window, uint32_t x0, uint32_t y0, uint32_t radius, uint32_t color);
void window_draw_text(window_t* window, uint32_t x, uint32_t y, const char* text, uint32_t color);
void window_clear(window_t* window, uint32_t color);

// Coordinate conversion
void window_client_to_screen(window_t* window, uint32_t client_x, uint32_t client_y, uint32_t* screen_x, uint32_t* screen_y);
void window_screen_to_client(window_t* window, uint32_t screen_x, uint32_t screen_y, uint32_t* client_x, uint32_t* client_y);

// Control management
int window_add_control(window_t* window, window_control_t* control);

// Standard controls
window_control_t* control_create_button(window_t* parent, uint32_t id, const char* text, 
                                      uint32_t x, uint32_t y, uint32_t width, uint32_t height);
                                      
window_control_t* control_create_label(window_t* parent, uint32_t id, const char* text, 
                                     uint32_t x, uint32_t y, uint32_t width, uint32_t height);
                                     
window_control_t* control_create_textbox(window_t* parent, uint32_t id, const char* text, 
                                       uint32_t x, uint32_t y, uint32_t width, uint32_t height);
                                       
window_control_t* control_create_checkbox(window_t* parent, uint32_t id, const char* text, 
                                        uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
                                        uint8_t checked);
                                        
window_control_t* control_create_radiobutton(window_t* parent, uint32_t id, const char* text, 
                                           uint32_t x, uint32_t y, uint32_t width, uint32_t height, 
                                           uint8_t checked);
                                           
window_control_t* control_create_listbox(window_t* parent, uint32_t id, 
                                       uint32_t x, uint32_t y, uint32_t width, uint32_t height);
                                       
// Control functions
void control_destroy(window_control_t* control);
void control_set_text(window_control_t* control, const char* text);
const char* control_get_text(window_control_t* control);
void control_set_checked(window_control_t* control, uint8_t checked);
uint8_t control_get_checked(window_control_t* control);
void control_set_enabled(window_control_t* control, uint8_t enabled);
void control_set_visible(window_control_t* control, uint8_t visible);
void control_set_focus(window_control_t* control);
void control_invalidate(window_control_t* control);

// Listbox-specific functions
void control_listbox_add_item(window_control_t* control, const char* text);
void control_listbox_clear(window_control_t* control);
void control_listbox_set_selected(window_control_t* control, int32_t index);
int32_t control_listbox_get_selected(window_control_t* control);
const char* control_listbox_get_selected_text(window_control_t* control);

#endif // GUI_WINDOW_H