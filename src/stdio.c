// src/stdio.c
#include "stdio.h"
#include "terminal.h"
#include <stdarg.h>

static void print_int(int n) {
    char buffer[16];
    int i = 0;
    int negative = 0;
    
    if (n < 0) {
        negative = 1;
        n = -n;
    }
    
    // Handle special case of 0
    if (n == 0) {
        terminal_putchar('0');
        return;
    }
    
    // Convert to string in reverse order
    while (n > 0) {
        buffer[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    if (negative)
        terminal_putchar('-');
    
    // Print in correct order
    while (i > 0)
        terminal_putchar(buffer[--i]);
}

int sscanf(const char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int count = 0;
    int str_i = 0;
    
    for (int i = 0; format[i] != '\0'; i++) {
        // Skip whitespace in format and string
        while (format[i] == ' ' || format[i] == '\t' || format[i] == '\n')
            i++;
        while (str[str_i] == ' ' || str[str_i] == '\t' || str[str_i] == '\n')
            str_i++;
        
        if (format[i] == '\0')
            break;
        
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case 's': {
                    char* s = va_arg(args, char*);
                    int j = 0;
                    while (str[str_i] && str[str_i] != ' ' && str[str_i] != '\t' && str[str_i] != '\n') {
                        s[j++] = str[str_i++];
                    }
                    s[j] = '\0';
                    count++;
                    break;
                }
                case 'd': {
                    int* d = va_arg(args, int*);
                    int val = 0;
                    int negative = 0;
                    
                    if (str[str_i] == '-') {
                        negative = 1;
                        str_i++;
                    }
                    
                    while (str[str_i] >= '0' && str[str_i] <= '9') {
                        val = val * 10 + (str[str_i] - '0');
                        str_i++;
                    }
                    
                    *d = negative ? -val : val;
                    count++;
                    break;
                }
            }
        } else {
            if (format[i] != str[str_i])
                break;
            str_i++;
        }
    }
    
    va_end(args);
    return count;
}

void terminal_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case 'd': {
                    int val = va_arg(args, int);
                    print_int(val);
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    terminal_writestring(s);
                    break;
                }
                case 'c': {
                    // Note: char is promoted to int when passed through ...
                    char c = (char)va_arg(args, int);
                    terminal_putchar(c);
                    break;
                }
                case '%':
                    terminal_putchar('%');
                    break;
                default:
                    terminal_putchar('%');
                    terminal_putchar(format[i]);
                    break;
            }
        } else {
            terminal_putchar(format[i]);
        }
    }
    
    va_end(args);
}