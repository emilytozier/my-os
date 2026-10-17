#include "serial.h"

#define COM1_PORT 0x3F8

static inline void outb(unsigned short port, unsigned char value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static int is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20; 
}


static int serial_received(void) {
    return inb(COM1_PORT + 5) & 1; 
}


void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);  
    outb(COM1_PORT + 3, 0x80);    
    outb(COM1_PORT + 0, 0x03);    
    outb(COM1_PORT + 1, 0x00);    
    outb(COM1_PORT + 3, 0x03);   
    outb(COM1_PORT + 2, 0xC7); 
    outb(COM1_PORT + 4, 0x0B);    
}


void print_serial_char(char c) {
    while (!is_transmit_empty());
    outb(COM1_PORT, c);
}


void print_serial(const char* str) {
    while (*str) {
        print_serial_char(*str++);
    }
}

char read_serial(void) { 
    while (!serial_received());
    return inb(COM1_PORT);
}

void serial_write(char c) {
    print_serial_char(c);
}
