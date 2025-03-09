// src/string.c
#include "string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while ((*dest++ = *src++));
    return original_dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
        dest[i] = src[i];
    for (; i < n; i++)
        dest[i] = '\0';
    return dest;
}

char* strncat(char* dest, const char* src, size_t n) {
    size_t dest_len = strlen(dest);
    size_t i;

    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';

    return dest;
}

char* strrchr(const char* s, int c) {
    const char* found = NULL;
    
    while (*s) {
        if (*s == (char)c) {
            found = s;
        }
        s++;
    }
    
    if (c == 0) {
        return (char*)s;  // Special case for null terminator
    }
    
    return (char*)found;
}

void* memcpy(void* dest, const void* src, size_t n) {
    char* d = dest;
    const char* s = src;
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
    return dest;
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    for (size_t i = 0; i < n; i++)
        p[i] = (unsigned char)c;
    return s;
}
// Compare n characters of two strings
int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i])
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        if (s1[i] == '\0')
            return 0;
    }
    return 0;
}

// Add to string.c
int atoi(const char* str) {
    int value = 0;
    int sign = 1;
    
    // Skip leading whitespace
    while (*str == ' ' || *str == '\t')
        str++;
    
    // Handle sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    // Convert digits
    while (*str >= '0' && *str <= '9') {
        value = value * 10 + (*str - '0');
        str++;
    }
    
    return value * sign;
}

// Find first occurrence of character c in string s
char* strchr(const char* s, int c) {
    while (*s != '\0') {
        if (*s == (char)c)
            return (char*)s;
        s++;
    }
    
    if ((char)c == '\0')
        return (char*)s;
    
    return NULL;
}
// Concatenate src to dest
char* strcat(char* dest, const char* src) {
    char* original_dest = dest;
    
    // Find end of dest
    while (*dest)
        dest++;
    
    // Copy src to dest
    while ((*dest++ = *src++))
        ;
    
    return original_dest;
}