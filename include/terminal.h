#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_writestring(const char* data);
void terminal_clear(void);  // Added terminal_clear function declaration

#endif