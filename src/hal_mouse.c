// src/hal_mouse.c
#include "hal_mouse.h"
#include "hal.h"  // Added to get hal_device_t definition
#include "terminal.h"
#include "stdio.h"

// PS/2 mouse ports
#define PS2_DATA_PORT      0x60
#define PS2_COMMAND_PORT   0x64
#define PS2_STATUS_PORT    0x64

// PS/2 mouse commands
#define PS2_CMD_READ_CONFIG  0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_DISABLE_AUX  0xA7
#define PS2_CMD_ENABLE_AUX   0xA8
#define PS2_CMD_MOUSE_CMD    0xD4

// PS/2 mouse device commands
#define MOUSE_CMD_RESET           0xFF
#define MOUSE_CMD_RESEND          0xFE
#define MOUSE_CMD_SET_DEFAULTS    0xF6
#define MOUSE_CMD_DISABLE_PACKETS 0xF5
#define MOUSE_CMD_ENABLE_PACKETS  0xF4
#define MOUSE_CMD_SET_SAMPLE_RATE 0xF3
#define MOUSE_CMD_GET_DEVICE_ID   0xF2
#define MOUSE_CMD_SET_REMOTE_MODE 0xF0
#define MOUSE_CMD_SET_WRAP_MODE   0xEE
#define MOUSE_CMD_SET_STREAM_MODE 0xEA
#define MOUSE_CMD_STATUS_REQUEST  0xE9
#define MOUSE_CMD_SET_RESOLUTION  0xE8
#define MOUSE_CMD_SET_SCALING_2_1 0xE7
#define MOUSE_CMD_SET_SCALING_1_1 0xE6

// PS/2 mouse responses
#define MOUSE_RES_ACK             0xFA
#define MOUSE_RES_SELF_TEST_PASS  0xAA
#define MOUSE_RES_ERROR           0xFC
#define MOUSE_RES_RESEND          0xFE

// Mouse data packet bits (first byte)
#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_RIGHT_BUTTON  0x02
#define MOUSE_MIDDLE_BUTTON 0x04
#define MOUSE_X_SIGN        0x10
#define MOUSE_Y_SIGN        0x20
#define MOUSE_X_OVERFLOW    0x40
#define MOUSE_Y_OVERFLOW    0x80

// Mouse types
#define MOUSE_TYPE_STANDARD 0
#define MOUSE_TYPE_WHEEL    3
#define MOUSE_TYPE_5BUTTON  4

// Mouse data structure
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;  // Wheel
    uint8_t buttons;
    uint8_t type;
    
    // Packet data
    uint8_t packet[4];
    uint8_t packet_index;
    uint8_t packet_size;
    uint8_t has_wheel;
} mouse_data_t;

// Local mouse device
static mouse_data_t mouse_data = {0};
static hal_device_t mouse_device = {0};

// For GUI integration - Mouse event callbacks
static mouse_event_handler_t mouse_event_handlers[MAX_MOUSE_EVENT_HANDLERS] = {0};
static uint8_t mouse_event_handler_count = 0;

// Write to PS/2 controller command port
static void ps2_write_command(uint8_t command) {
    // Wait for input buffer to be empty
    while (inb(PS2_STATUS_PORT) & 0x02);
    outb(PS2_COMMAND_PORT, command);
}

// Write to PS/2 data port
static void ps2_write_data(uint8_t data) {
    // Wait for input buffer to be empty
    while (inb(PS2_STATUS_PORT) & 0x02);
    outb(PS2_DATA_PORT, data);
}

// Read from PS/2 data port with timeout
static int ps2_read_data_timeout(uint8_t* data, uint32_t timeout) {
    for (uint32_t i = 0; i < timeout; i++) {
        // Check if output buffer is full
        if (inb(PS2_STATUS_PORT) & 0x01) {
            *data = inb(PS2_DATA_PORT);
            return 0;
        }
        
        // Small delay (about 1 microsecond)
        for (volatile int j = 0; j < 1000; j++);
    }
    
    return -1;  // Timeout
}

// Send command to mouse
static int mouse_send_command(uint8_t command) {
    for (int i = 0; i < 3; i++) {  // Try up to 3 times
        // Send "Send to auxiliary device" command
        ps2_write_command(PS2_CMD_MOUSE_CMD);
        
        // Send actual mouse command
        ps2_write_data(command);
        
        // Wait for acknowledgment
        uint8_t response;
        if (ps2_read_data_timeout(&response, 1000) == 0) {
            if (response == MOUSE_RES_ACK) {
                return 0;  // Success
            }
            
            if (response == MOUSE_RES_RESEND) {
                continue;  // Try again
            }
        }
    }
    
    return -1;  // Failed
}

// Send command to mouse with data byte
static int mouse_send_command_with_data(uint8_t command, uint8_t data) {
    if (mouse_send_command(command) == 0) {
        // Send data byte
        return mouse_send_command(data);
    }
    
    return -1;
}

// Initialize PS/2 mouse
static int init_ps2_mouse(void) {
    // Save old command byte
    ps2_write_command(PS2_CMD_READ_CONFIG);
    uint8_t config;
    if (ps2_read_data_timeout(&config, 1000) != 0) {
        terminal_writestring("PS/2: Failed to read config\n");
        return -1;
    }
    
    // Disable PS/2 devices during initialization
    ps2_write_command(PS2_CMD_DISABLE_AUX);
    
    // Enable auxiliary device, interrupts
    config |= 0x02;  // Enable auxiliary device interrupt
    ps2_write_command(PS2_CMD_WRITE_CONFIG);
    ps2_write_data(config);
    
    // Enable auxiliary device
    ps2_write_command(PS2_CMD_ENABLE_AUX);
    
    // Reset mouse
    if (mouse_send_command(MOUSE_CMD_RESET) != 0) {
        terminal_writestring("PS/2 Mouse: Reset failed\n");
        return -1;
    }
    
    // Wait for self-test response
    uint8_t response;
    if (ps2_read_data_timeout(&response, 1000) != 0 || response != MOUSE_RES_SELF_TEST_PASS) {
        terminal_writestring("PS/2 Mouse: Self-test failed\n");
        return -1;
    }
    
    // Read device ID (should be 0x00 for standard PS/2 mouse)
    if (mouse_send_command(MOUSE_CMD_GET_DEVICE_ID) != 0) {
        terminal_writestring("PS/2 Mouse: Get device ID failed\n");
        return -1;
    }
    
    if (ps2_read_data_timeout(&response, 1000) != 0) {
        terminal_writestring("PS/2 Mouse: No response to device ID\n");
        return -1;
    }
    
    // Set mouse type based on device ID
    mouse_data.type = response;
    
    // Check if mouse has a wheel
    if (mouse_data.type == 0x00) {
        // Try to enable wheel mouse by setting sample rate
        mouse_send_command(MOUSE_CMD_SET_SAMPLE_RATE);
        mouse_send_command(200);
        mouse_send_command(MOUSE_CMD_SET_SAMPLE_RATE);
        mouse_send_command(100);
        mouse_send_command(MOUSE_CMD_SET_SAMPLE_RATE);
        mouse_send_command(80);
        
        // Ask for ID again
        mouse_send_command(MOUSE_CMD_GET_DEVICE_ID);
        ps2_read_data_timeout(&response, 1000);
        
        if (response == MOUSE_TYPE_WHEEL || response == MOUSE_TYPE_5BUTTON) {
            mouse_data.type = response;
            mouse_data.has_wheel = 1;
            mouse_data.packet_size = 4;
        } else {
            mouse_data.has_wheel = 0;
            mouse_data.packet_size = 3;
        }
    } else if (mouse_data.type == MOUSE_TYPE_WHEEL || mouse_data.type == MOUSE_TYPE_5BUTTON) {
        mouse_data.has_wheel = 1;
        mouse_data.packet_size = 4;
    } else {
        mouse_data.has_wheel = 0;
        mouse_data.packet_size = 3;
    }
    
    // Set default parameters
    mouse_send_command(MOUSE_CMD_SET_DEFAULTS);
    
    // Set sample rate (how often the mouse sends updates)
    mouse_send_command_with_data(MOUSE_CMD_SET_SAMPLE_RATE, 100);
    
    // Set resolution (counts per inch)
    mouse_send_command_with_data(MOUSE_CMD_SET_RESOLUTION, 2);  // 4 counts/mm
    
    // Enable data reporting
    mouse_send_command(MOUSE_CMD_ENABLE_PACKETS);
    
    terminal_printf("PS/2 Mouse initialized, type: %d, wheel: %s\n", 
                  mouse_data.type, 
                  mouse_data.has_wheel ? "yes" : "no");
    
    return 0;
}

// Process a complete mouse packet
static void process_mouse_packet(void) {
    // First byte contains button state and signs
    uint8_t buttons = mouse_data.packet[0] & 0x07;  // 3 lower bits are buttons
    
    // Extract movement values
    int16_t dx = mouse_data.packet[1];
    int16_t dy = mouse_data.packet[2];
    
    // Apply sign bit
    if (mouse_data.packet[0] & MOUSE_X_SIGN) {
        dx |= 0xFF00;  // Sign extend to 16 bits
    }
    
    if (mouse_data.packet[0] & MOUSE_Y_SIGN) {
        dy |= 0xFF00;  // Sign extend to 16 bits
    }
    
    // Y movement is inverted in PS/2 mouse
    dy = -dy;
    
    // Extract wheel movement if available
    int16_t dz = 0;
    if (mouse_data.has_wheel && mouse_data.packet_size >= 4) {
        // Wheel data is in the 4th byte, as a signed value
        dz = (int8_t)mouse_data.packet[3];
    }
    
    // Update mouse position
    mouse_data.x += dx;
    mouse_data.y += dy;
    mouse_data.z += dz;
    
    // Clamp position to screen boundaries (assuming 640x480)
    if (mouse_data.x < 0) mouse_data.x = 0;
    if (mouse_data.y < 0) mouse_data.y = 0;
    if (mouse_data.x > 639) mouse_data.x = 639;
    if (mouse_data.y > 479) mouse_data.y = 479;
    
    // Update button state
    mouse_data.buttons = buttons;
    
    // Call event handlers
    mouse_event_t event = {
        .x = mouse_data.x,
        .y = mouse_data.y,
        .z = dz,
        .dx = dx,
        .dy = dy,
        .dz = dz,
        .buttons = buttons,
        .prev_buttons = mouse_data.buttons
    };
    
    for (uint8_t i = 0; i < mouse_event_handler_count; i++) {
        if (mouse_event_handlers[i] != NULL) {
            mouse_event_handlers[i](&event);
        }
    }
}

// Poll for mouse data
static void mouse_poll(void) {
    // Check if data is available
    if (inb(PS2_STATUS_PORT) & 0x01) {
        uint8_t data = inb(PS2_DATA_PORT);
        
        // Handle packet assembly
        if (mouse_data.packet_index == 0 && (data & 0x08) == 0) {
            // Ignore packet if first byte doesn't have bit 3 set
            // (synchronization)
            return;
        }
        
        // Add byte to packet
        mouse_data.packet[mouse_data.packet_index++] = data;
        
        // Process packet if complete
        if (mouse_data.packet_index >= mouse_data.packet_size) {
            process_mouse_packet();
            mouse_data.packet_index = 0;
        }
    }
}

// Device-specific functions
static int mouse_init(void* device) {
    // Initialize PS/2 mouse
    if (init_ps2_mouse() != 0) {
        terminal_writestring("Failed to initialize PS/2 mouse\n");
        return -1;
    }
    
    // Initialize mouse data
    mouse_data.x = 320;  // Center of 640x480 screen
    mouse_data.y = 240;
    mouse_data.z = 0;
    mouse_data.buttons = 0;
    mouse_data.packet_index = 0;
    
    terminal_writestring("HAL Mouse initialized\n");
    
    return 0;
}

static int mouse_close(void* device) {
    // Disable mouse data reporting
    mouse_send_command(MOUSE_CMD_DISABLE_PACKETS);
    
    return 0;
}

static int mouse_read(void* device, void* buffer, uint32_t size) {
    mouse_state_t* state = (mouse_state_t*)buffer;
    
    if (size < sizeof(mouse_state_t)) {
        return -1;
    }
    
    // Poll for new data
    mouse_poll();
    
    // Copy current state
    state->x = mouse_data.x;
    state->y = mouse_data.y;
    state->z = mouse_data.z;
    state->buttons = mouse_data.buttons;
    
    return sizeof(mouse_state_t);
}

static int mouse_write(void* device, const void* buffer, uint32_t size) {
    // Mouse is read-only
    return -1;
}

static int mouse_ioctl(void* device, uint32_t request, void* arg) {
    switch (request) {
        case MOUSE_IOCTL_GET_STATE: {
            // Get mouse state
            mouse_state_t* state = (mouse_state_t*)arg;
            if (state) {
                state->x = mouse_data.x;
                state->y = mouse_data.y;
                state->z = mouse_data.z;
                state->buttons = mouse_data.buttons;
                return 0;
            }
            break;
        }
        case MOUSE_IOCTL_SET_POSITION: {
            // Set mouse position
            mouse_pos_t* pos = (mouse_pos_t*)arg;
            if (pos) {
                mouse_data.x = pos->x;
                mouse_data.y = pos->y;
                return 0;
            }
            break;
        }
        case MOUSE_IOCTL_REGISTER_HANDLER: {
            // Register event handler
            mouse_event_handler_t handler = (mouse_event_handler_t)arg;
            if (handler && mouse_event_handler_count < MAX_MOUSE_EVENT_HANDLERS) {
                mouse_event_handlers[mouse_event_handler_count++] = handler;
                return 0;
            }
            break;
        }
        case MOUSE_IOCTL_UNREGISTER_HANDLER: {
            // Unregister event handler
            mouse_event_handler_t handler = (mouse_event_handler_t)arg;
            if (handler) {
                for (uint8_t i = 0; i < mouse_event_handler_count; i++) {
                    if (mouse_event_handlers[i] == handler) {
                        // Remove handler by shifting remaining ones
                        for (uint8_t j = i; j < mouse_event_handler_count - 1; j++) {
                            mouse_event_handlers[j] = mouse_event_handlers[j + 1];
                        }
                        mouse_event_handlers[--mouse_event_handler_count] = NULL;
                        return 0;
                    }
                }
            }
            break;
        }
    }
    
    return -1;  // Invalid request or argument
}

// HAL mouse interface functions

void mouse_get_state(mouse_state_t* state) {
    mouse_ioctl(&mouse_device, MOUSE_IOCTL_GET_STATE, state);
}

void mouse_set_position(int16_t x, int16_t y) {
    mouse_pos_t pos = { x, y };
    mouse_ioctl(&mouse_device, MOUSE_IOCTL_SET_POSITION, &pos);
}

int mouse_register_handler(mouse_event_handler_t handler) {
    return mouse_ioctl(&mouse_device, MOUSE_IOCTL_REGISTER_HANDLER, handler);
}

int mouse_unregister_handler(mouse_event_handler_t handler) {
    return mouse_ioctl(&mouse_device, MOUSE_IOCTL_UNREGISTER_HANDLER, handler);
}

void mouse_update(void) {
    // Poll for new mouse data
    mouse_poll();
}

// Initialize and register mouse device
int hal_mouse_init(void) {
    // Setup device
    mouse_device.type = HAL_DEVICE_MOUSE;
    mouse_device.mode = HAL_MODE_POLLING;
    mouse_device.private_data = &mouse_data;
    
    // Set functions
    mouse_device.init = mouse_init;
    mouse_device.close = mouse_close;
    mouse_device.read = mouse_read;
    mouse_device.write = mouse_write;
    mouse_device.ioctl = mouse_ioctl;
    
    // Register with HAL
    int result = hal_register_device(&mouse_device);
    if (result != 0) {
        terminal_writestring("Failed to register mouse device\n");
        return result;
    }
    
    // Initialize the device
    result = mouse_init(&mouse_device);
    
    return result;
}