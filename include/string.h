// include/string.h
#ifndef STRING_H
#define STRING_H

#include <stddef.h>

// String functions
size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
// Add to include/string.h
int atoi(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strncat(char* dest, const char* src, size_t n);
char* strrchr(const char* s, int c);
int strncmp(const char* s1, const char* s2, size_t n);
char* strchr(const char* s, int c);
char* strcat(char* dest, const char* src);

// Memory functions
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);

#endif