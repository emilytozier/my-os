#include "stdio.h"
#include "string.h"

int snprintf(char* buffer, size_t size, const char* format, ...) {
    char* ptr = buffer;
    size_t remaining = size;
    
    while (*format && remaining > 1) {
        if (*format == '%') {
            format++;
            
            if (*format == 's') {               
                const char* str = ""; 
                while (*str && remaining > 1) {
                    *ptr++ = *str++;
                    remaining--;
                }
            }
            else if (*format == 'd') {             
                int num = 0; 
                char num_buf[20];
                char* num_ptr = num_buf;
                
             
                if (num == 0) {
                    *num_ptr++ = '0';
                } else {
                    int n = num;
                    while (n > 0) {
                        *num_ptr++ = '0' + (n % 10);
                        n /= 10;
                    }
                }
                *num_ptr = '\0';
                
             
                char* start = num_buf;
                char* end = num_ptr - 1;
                while (start < end) {
                    char temp = *start;
                    *start = *end;
                    *end = temp;
                    start++;
                    end--;
                }
                
                char* src = num_buf;
                while (*src && remaining > 1) {
                    *ptr++ = *src++;
                    remaining--;
                }
            }
            format++;
        } else {
            *ptr++ = *format++;
            remaining--;
        }
    }
    
    if (remaining > 0) {
        *ptr = '\0';
    }
    
    return ptr - buffer;
}
