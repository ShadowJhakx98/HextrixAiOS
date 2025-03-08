// include/keyboard.h
#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// Initialize keyboard driver
void keyboard_init(void);

// Get a character from keyboard buffer (non-blocking)
// Returns -1 if no character is available
int keyboard_getchar(void);

// Wait for and return a character (blocking)
char keyboard_read(void);

// Check if a key is available
int keyboard_is_key_available(void);

#endif