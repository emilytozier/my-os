#include "screen.h"
#include "string.h"
#include "serial.h"
#include <stdarg.h>  

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BUFFER 0xB8000


volatile unsigned short* vga_buffer = (volatile unsigned short*)VGA_BUFFER;

static int cursor_x = 0;
static int cursor_y = 0;
static unsigned char color = 0x0F;

static void print_number(int num, int base) {
    char buffer[32];
    int i = 0;
    int is_negative = 0;
    
    if (num == 0) {
        print_char('0');
        return;
    }
  
    if (base == 10 && num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    while (num > 0) {
        int digit = num % base;
        buffer[i++] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        num /= base;
    }
    

    if (is_negative) {
        print_char('-');
    }

    while (--i >= 0) {
        print_char(buffer[i]);
    }
}

void clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (unsigned short)(' ' | (color << 8));
    }
    cursor_x = 0;
    cursor_y = 0;
}

void set_color(unsigned char foreground, unsigned char background) {
    color = (background << 4) | (foreground & 0x0F);
}

int get_cursor_y(void) {
    return cursor_y;
}

void put_char(char c, int x, int y) {
    vga_buffer[y * VGA_WIDTH + x] = (unsigned short)(c | (color << 8));
}

void scroll_screen(void) {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y-1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];
        }
    }

    for (int x = 0; x < VGA_WIDTH; x++) {
        put_char(' ', x, VGA_HEIGHT - 1);
    }
}

void print_char(char c) {
    if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            put_char(' ', cursor_x, cursor_y);
        }
        return;
    }  

   if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        put_char(c, cursor_x, cursor_y);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        scroll_screen();
        cursor_y = VGA_HEIGHT - 1;
    }
}

void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char c;
    const char* s;
    int d;
    
    while ((c = *format++) != '\0') {
        if (c != '%') {
            print_char(c);
            continue;
        }
        
        c = *format++;
        switch (c) {
            case 'c':
                print_char((char)va_arg(args, int));
                break;
                
            case 's':
                s = va_arg(args, const char*);
                while (*s) {
                    print_char(*s++);
                }
                break;
                
            case 'd':
            case 'i':
                d = va_arg(args, int);
                print_number(d, 10);
                break;
                
            case 'x':
                d = va_arg(args, int);
                print_char('0');
                print_char('x');
                print_number(d, 16);
                break;
                
            case '%':
                print_char('%');
                break;
                
            default:
                print_char('%');
                print_char(c);
                break;
        }
    }
    
    va_end(args);
}


void print_char_at(char c, int x, int y) {
    volatile unsigned short* vga_buffer = (volatile unsigned short*)0xB8000;
    vga_buffer[y * 80 + x] = (unsigned short)(c | (color << 8));
}

void print_string(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

