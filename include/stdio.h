// include/stdio.h
#ifndef STDIO_H
#define STDIO_H

// Simple printf implementation for terminal output
void terminal_printf(const char* format, ...);

// Simple sscanf implementation to parse strings
int sscanf(const char* str, const char* format, ...);

// Simple sprintf implementation to format strings
int sprintf(char* str, const char* format, ...);

#endif