// Add this to include/stdio.h if not already there

#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>

// Simple printf implementation for terminal output
void terminal_printf(const char* format, ...);

// Simple sprintf implementation to format strings
int sprintf(char* str, const char* format, ...);

// Simple snprintf implementation with buffer size limit
int snprintf(char* str, size_t size, const char* format, ...);

// Simple sscanf implementation to parse strings
int sscanf(const char* str, const char* format, ...);

#endif // STDIO_H