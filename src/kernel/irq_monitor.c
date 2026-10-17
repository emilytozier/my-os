#include "screen.h"

static inline void outb(unsigned short port, unsigned char value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}


void irq_monitor(void) {
    kprintf("\n=== IRQ MONITOR ACTIVE ===\n");
    kprintf("Monitoring IRQ0 (Timer) and IRQ1 (Keyboard)\n");
    
    unsigned int last_timer_count = 0;
    unsigned int timer_count = 0;
    unsigned int keyboard_count = 0;
    
    while(1) {
        
        outb(0x20, 0x0B); 
        unsigned char isr1 = inb(0x20);
        
        outb(0xA0, 0x0B); 
        unsigned char isr2 = inb(0xA0);        
      
        if (isr1 & 0x01) {
            timer_count++;
        }
       
        if (isr1 & 0x02) {
            keyboard_count++;
            kprintf("KEYBOARD IRQ DETECTED! Count: %d\n", keyboard_count);            
       
            unsigned char status = inb(0x64);
            if (status & 0x01) {
                int scancode = inb(0x60);
                kprintf("Keyboard data: scancode=0x%x\n", scancode);
            }
        }
        
        if (timer_count != last_timer_count && timer_count % 100 == 0) {
            kprintf("Timer: %d, Keyboard: %d, ISR1: 0x%x, ISR2: 0x%x\n", 
                   timer_count, keyboard_count, isr1, isr2);
            last_timer_count = timer_count;
        }
        
        for(int i = 0; i < 1000; i++) asm volatile ("nop");
    }
}
