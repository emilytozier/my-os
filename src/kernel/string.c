#include "string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;
        while (*h && *n && *h == *n) { h++; n++; }
        if (!*n) return (char*)haystack;
    }
    return NULL;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* ret = dest;
    while (n-- && (*dest++ = *src++));
    while (n--) *dest++ = '\0';
    return ret;
}

size_t strlcpy(char* dest, const char* src, size_t size) {
    size_t i;
    for (i = 0; i < size - 1 && src[i] != '\0'; i++) dest[i] = src[i];
    if (size > 0) dest[i] = '\0';
    while (src[i] != '\0') i++;
    return i;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--) *p++ = (unsigned char)value;
    return ptr;
}
