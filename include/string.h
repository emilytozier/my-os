#ifndef STRING_H
#define STRING_H

#include <stddef.h>

size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);

int starts_with(const char* str, const char* prefix);
char* strstr(const char* haystack, const char* needle);
char* strncpy(char* dest, const char* src, size_t n);
size_t strlcpy(char* dest, const char* src, size_t size);
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* ptr, int value, size_t num);

#endif
