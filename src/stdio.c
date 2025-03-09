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

// Implementation of sprintf for string formatting
int sprintf(char* str, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int written = 0;
    
    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case 'd': {
                    int val = va_arg(args, int);
                    
                    // Handle negative numbers
                    if (val < 0) {
                        str[written++] = '-';
                        val = -val;
                    }
                    
                    // Handle special case of 0
                    if (val == 0) {
                        str[written++] = '0';
                        break;
                    }
                    
                    // Convert to string in reverse order
                    char buffer[16];
                    int buf_pos = 0;
                    
                    while (val > 0) {
                        buffer[buf_pos++] = '0' + (val % 10);
                        val /= 10;
                    }
                    
                    // Reverse the digits
                    while (buf_pos > 0) {
                        str[written++] = buffer[--buf_pos];
                    }
                    
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (s == NULL) {
                        s = "(null)";
                    }
                    while (*s) {
                        str[written++] = *s++;
                    }
                    break;
                }
                case 'x': {
                    // Hex format
                    unsigned int val = va_arg(args, unsigned int);
                    
                    // Handle special case of 0
                    if (val == 0) {
                        str[written++] = '0';
                        break;
                    }
                    
                    // Convert to hex in reverse order
                    char buffer[16];
                    int buf_pos = 0;
                    
                    while (val > 0) {
                        int digit = val & 0xF;
                        if (digit < 10) {
                            buffer[buf_pos++] = '0' + digit;
                        } else {
                            buffer[buf_pos++] = 'a' + (digit - 10);
                        }
                        val >>= 4;
                    }
                    
                    // Reverse the digits
                    while (buf_pos > 0) {
                        str[written++] = buffer[--buf_pos];
                    }
                    
                    break;
                }
                case 'c': {
                    // Note: char is promoted to int when passed through ...
                    char c = (char)va_arg(args, int);
                    str[written++] = c;
                    break;
                }
                case '%':
                    str[written++] = '%';
                    break;
                default:
                    str[written++] = '%';
                    str[written++] = format[i];
                    break;
            }
        } else {
            str[written++] = format[i];
        }
    }
    
    // Null terminate the string
    str[written] = '\0';
    
    va_end(args);
    return written;
}

// Implementation of snprintf for size-limited string formatting
int snprintf(char* str, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    if (size == 0) {
        va_end(args);
        return 0;
    }
    
    int written = 0;
    size_t remaining = size - 1; // Reserve space for null terminator
    
    for (int i = 0; format[i] != '\0' && remaining > 0; i++) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case 'd': {
                    int val = va_arg(args, int);
                    
                    // Handle negative numbers
                    if (val < 0) {
                        if (remaining > 0) {
                            str[written++] = '-';
                            remaining--;
                        }
                        val = -val;
                    }
                    
                    // Handle special case of 0
                    if (val == 0) {
                        if (remaining > 0) {
                            str[written++] = '0';
                            remaining--;
                        }
                        break;
                    }
                    
                    // Convert to string in reverse order
                    char buffer[16];
                    int buf_pos = 0;
                    
                    while (val > 0) {
                        buffer[buf_pos++] = '0' + (val % 10);
                        val /= 10;
                    }
                    
                    // Reverse the digits
                    while (buf_pos > 0 && remaining > 0) {
                        str[written++] = buffer[--buf_pos];
                        remaining--;
                    }
                    
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (s == NULL) {
                        s = "(null)";
                    }
                    while (*s && remaining > 0) {
                        str[written++] = *s++;
                        remaining--;
                    }
                    break;
                }
                case 'c': {
                    // Note: char is promoted to int when passed through ...
                    char c = (char)va_arg(args, int);
                    if (remaining > 0) {
                        str[written++] = c;
                        remaining--;
                    }
                    break;
                }
                case '%':
                    if (remaining > 0) {
                        str[written++] = '%';
                        remaining--;
                    }
                    break;
                default:
                    if (remaining > 0) {
                        str[written++] = '%';
                        remaining--;
                    }
                    if (remaining > 0) {
                        str[written++] = format[i];
                        remaining--;
                    }
                    break;
            }
        } else {
            str[written++] = format[i];
            remaining--;
        }
    }
    
    // Null terminate the string
    str[written] = '\0';
    
    va_end(args);
    return written;
}

// Implementation of sscanf for string parsing
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

// Terminal printf implementation
void terminal_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++;  // Move past '%'
            
            // Handle width specification if present
            int width = 0;
            int padding_needed = 0;
            char padding_char = ' ';
            
            // Handle padding character (0-padding)
            if (format[i] == '0') {
                padding_char = '0';
                i++;
            }
            
            // Handle width specification
            if (format[i] >= '1' && format[i] <= '9') {
                width = format[i] - '0';
                i++;
                
                // Handle multi-digit width
                while (format[i] >= '0' && format[i] <= '9') {
                    width = width * 10 + (format[i] - '0');
                    i++;
                }
            }
            
            // Handle format specifiers
            switch (format[i]) {
                case 'd': {
                    // Integer
                    int val = va_arg(args, int);
                    
                    // Handle negative numbers
                    if (val < 0) {
                        terminal_putchar('-');
                        val = -val;
                        if (width > 0) width--; // Adjust width for minus sign
                    }
                    
                    // Handle special case of 0
                    if (val == 0) {
                        if (width > 0) {
                            for (int j = 0; j < width - 1; j++) {
                                terminal_putchar(padding_char);
                            }
                        }
                        terminal_putchar('0');
                        break;
                    }
                    
                    // Calculate number of digits
                    int temp = val;
                    int digits = 0;
                    while (temp > 0) {
                        digits++;
                        temp /= 10;
                    }
                    
                    // Add padding if needed
                    if (width > digits) {
                        for (int j = 0; j < width - digits; j++) {
                            terminal_putchar(padding_char);
                        }
                    }
                    
                    // Print the number in reverse order
                    char buffer[32];
                    int index = 0;
                    while (val > 0) {
                        buffer[index++] = '0' + (val % 10);
                        val /= 10;
                    }
                    
                    // Print in correct order
                    for (int j = index - 1; j >= 0; j--) {
                        terminal_putchar(buffer[j]);
                    }
                    break;
                }
                
                case 's': {
                    // String
                    const char* str = va_arg(args, const char*);
                    if (str) {
                        // Calculate length if width specified
                        if (width > 0) {
                            int len = 0;
                            while (str[len]) len++;
                            
                            // Add padding if needed
                            if (width > len) {
                                for (int j = 0; j < width - len; j++) {
                                    terminal_putchar(padding_char);
                                }
                            }
                        }
                        
                        // Output the string
                        while (*str) {
                            terminal_putchar(*str++);
                        }
                    } else {
                        // Handle NULL string
                        terminal_writestring("(null)");
                    }
                    break;
                }
                
                case 'c': {
                    // Character (char is promoted to int when passed through ...)
                    char c = (char)va_arg(args, int);
                    terminal_putchar(c);
                    break;
                }
                
                case 'x': {
                    // Hexadecimal
                    unsigned int val = va_arg(args, unsigned int);
                    
                    // Handle special case of 0
                    if (val == 0) {
                        if (width > 0) {
                            for (int j = 0; j < width - 1; j++) {
                                terminal_putchar(padding_char);
                            }
                        }
                        terminal_putchar('0');
                        break;
                    }
                    
                    // Calculate number of hex digits
                    unsigned int temp = val;
                    int digits = 0;
                    while (temp > 0) {
                        digits++;
                        temp >>= 4;
                    }
                    
                    // Add padding if needed
                    if (width > digits) {
                        for (int j = 0; j < width - digits; j++) {
                            terminal_putchar(padding_char);
                        }
                    }
                    
                    // Print the number in reverse order
                    char buffer[32];
                    int index = 0;
                    while (val > 0) {
                        int digit = val & 0xF;
                        if (digit < 10) {
                            buffer[index++] = '0' + digit;
                        } else {
                            buffer[index++] = 'a' + (digit - 10);
                        }
                        val >>= 4;
                    }
                    
                    // Print in correct order
                    for (int j = index - 1; j >= 0; j--) {
                        terminal_putchar(buffer[j]);
                    }
                    break;
                }
                
                case '*': {
                    // Width is passed as an argument
                    width = va_arg(args, int);
                    // Continue to process the next character
                    i++;
                    break;
                }
                
                case '%':
                    // Literal %
                    terminal_putchar('%');
                    break;
                    
                default:
                    // Unknown format, just output it
                    terminal_putchar('%');
                    terminal_putchar(format[i]);
                    break;
            }
        } else {
            // Regular character
            terminal_putchar(format[i]);
        }
    }
    
    va_end(args);
}