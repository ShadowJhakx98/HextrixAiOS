// include/hal_keyboard.h
#ifndef HAL_KEYBOARD_H
#define HAL_KEYBOARD_H

#include <stdint.h>

// Key codes
#define KEY_ESC     0x01
#define KEY_1       0x02
#define KEY_2       0x03
#define KEY_3       0x04
#define KEY_4       0x05
#define KEY_5       0x06
#define KEY_6       0x07
#define KEY_7       0x08
#define KEY_8       0x09
#define KEY_9       0x0A
#define KEY_0       0x0B
#define KEY_MINUS   0x0C
#define KEY_EQUALS  0x0D
#define KEY_BKSP    0x0E
#define KEY_TAB     0x0F
#define KEY_Q       0x10
#define KEY_W       0x11
#define KEY_E       0x12
#define KEY_R       0x13
#define KEY_T       0x14
#define KEY_Y       0x15
#define KEY_U       0x16
#define KEY_I       0x17
#define KEY_O       0x18
#define KEY_P       0x19
#define KEY_LBRACE  0x1A
#define KEY_RBRACE  0x1B
#define KEY_ENTER   0x1C
#define KEY_LCTRL   0x1D
#define KEY_A       0x1E
#define KEY_S       0x1F
#define KEY_D       0x20
#define KEY_F       0x21
#define KEY_G       0x22
#define KEY_H       0x23
#define KEY_J       0x24
#define KEY_K       0x25
#define KEY_L       0x26
#define KEY_SCOLON  0x27
#define KEY_QUOTE   0x28
#define KEY_BQUOTE  0x29
#define KEY_LSHIFT  0x2A
#define KEY_BSLASH  0x2B
#define KEY_Z       0x2C
#define KEY_X       0x2D
#define KEY_C       0x2E
#define KEY_V       0x2F
#define KEY_B       0x30
#define KEY_N       0x31
#define KEY_M       0x32
#define KEY_COMMA   0x33
#define KEY_DOT     0x34
#define KEY_SLASH   0x35
#define KEY_RSHIFT  0x36
#define KEY_STAR    0x37
#define KEY_LALT    0x38
#define KEY_SPACE   0x39
#define KEY_CAPS    0x3A
#define KEY_F1      0x3B
#define KEY_F2      0x3C
#define KEY_F3      0x3D
#define KEY_F4      0x3E
#define KEY_F5      0x3F
#define KEY_F6      0x40
#define KEY_F7      0x41
#define KEY_F8      0x42
#define KEY_F9      0x43
#define KEY_F10     0x44

// Special key flag (for key release)
#define KEY_RELEASE 0x80

// Initialize the keyboard subsystem
int hal_keyboard_init(void);

// Read a key from the keyboard
// Returns the scancode or -1 if no key is available
int hal_keyboard_read(void);

// Check if a key is available
// Returns 1 if a key is available, 0 otherwise
int hal_keyboard_is_key_available(void);

// Poll the keyboard hardware (used in polling mode)
void hal_keyboard_poll(void);

// Convert scancode to ASCII character
static inline char hal_keyboard_scancode_to_ascii(int scancode) {
    // Remove release bit if present
    scancode &= ~KEY_RELEASE;
    
    // Basic scancode to ASCII mapping
    static const char ascii_table[128] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' '
    };
    
    // Return ASCII character if in range
    if (scancode < 0 || scancode >= 128) {
        return 0;
    }
    
    return ascii_table[scancode];
}

#endif // HAL_KEYBOARD_H