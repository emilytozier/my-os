#include "screen.h"
#include "debug.h"  

static inline void outb(unsigned short port, unsigned char value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void test_interrupts_setup(void) {
    kprintf("Testing interrupts setup...\n");
    
    outb(0x21, 0x00);
    unsigned char pic1_mask = inb(0x21);
    kprintf("PIC1 mask: 0x%x\n", pic1_mask);
    
    outb(0xA1, 0x00);
    unsigned char pic2_mask = inb(0xA1);
    kprintf("PIC2 mask: 0x%x\n", pic2_mask);
    
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);
    
    kprintf("PIC masks set: IRQ1 (keyboard) enabled\n");
}
