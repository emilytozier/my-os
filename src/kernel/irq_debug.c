#include "screen.h"

static inline void outb(unsigned short port, unsigned char value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static int irq_count[16] = {0};

void debug_irq_handler(int irq_num) {
    irq_count[irq_num]++;
    
    kprintf("IRQ%d - Count: %d\n", irq_num, irq_count[irq_num]);
    
    if (irq_num == 0) {
        kprintf("TIMER IRQ - This should NOT cause reboot!\n");
    }
  
    if (irq_num >= 8) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);
}
