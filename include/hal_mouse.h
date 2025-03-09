// include/hal_mouse.h
#ifndef HAL_MOUSE_H
#define HAL_MOUSE_H

#include <stdint.h>

// Add this to include/hal.h (device types section)
// #define HAL_DEVICE_MOUSE 6

// Maximum number of mouse event handlers
#define MAX_MOUSE_EVENT_HANDLERS 8

// Mouse button definitions
#define MOUSE_BUTTON_LEFT   0x01
#define MOUSE_BUTTON_RIGHT  0x02
#define MOUSE_BUTTON_MIDDLE 0x04
#define MOUSE_BUTTON_EXTRA1 0x08
#define MOUSE_BUTTON_EXTRA2 0x10

// IOCTL commands for mouse
#define MOUSE_IOCTL_GET_STATE         0
#define MOUSE_IOCTL_SET_POSITION      1
#define MOUSE_IOCTL_REGISTER_HANDLER  2
#define MOUSE_IOCTL_UNREGISTER_HANDLER 3

// Mouse state structure
typedef struct {
    int16_t x;        // X position
    int16_t y;        // Y position
    int16_t z;        // Wheel position
    uint8_t buttons;  // Button state
} mouse_state_t;

// Mouse position structure
typedef struct {
    int16_t x;
    int16_t y;
} mouse_pos_t;

// Mouse event structure
typedef struct {
    int16_t x;           // Current X position
    int16_t y;           // Current Y position
    int16_t z;           // Current wheel position
    int16_t dx;          // X movement
    int16_t dy;          // Y movement
    int16_t dz;          // Wheel movement
    uint8_t buttons;     // Current button state
    uint8_t prev_buttons; // Previous button state
} mouse_event_t;

// Mouse event handler function type
typedef void (*mouse_event_handler_t)(mouse_event_t* event);

// Initialize mouse
int hal_mouse_init(void);

// Get current mouse state
void mouse_get_state(mouse_state_t* state);

// Set mouse position
void mouse_set_position(int16_t x, int16_t y);

// Register mouse event handler
int mouse_register_handler(mouse_event_handler_t handler);

// Unregister mouse event handler
int mouse_unregister_handler(mouse_event_handler_t handler);

// Poll for mouse updates
void mouse_update(void);

#endif // HAL_MOUSE_H