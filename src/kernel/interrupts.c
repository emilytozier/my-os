#include "keyboard.h"
#include "interrupts.h"
#include "screen.h"
#include "serial.h"
#include "process.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

static struct idt_entry idt[256];
static struct idt_ptr   idtp;

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

extern void isr0();
extern void isr1();
extern void isr6();
extern void isr8();
extern void isr13();
extern void isr14();
extern void irq0();
extern void irq1();

void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector  = sel;
    idt[num].zero      = 0;
    idt[num].flags     = flags;
}

static void load_idt(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;
    asm volatile ("lidt %0" : : "m"(idtp));
}

static void pic_remap(void) {
    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);
    outb(PIC1_DATA, 0xFC);
    outb(PIC2_DATA, 0xFF);
}

static void timer_init(uint32_t frequency) {
    uint32_t divisor = 1193180 / frequency;
    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
}

void isr_handler(struct regs* r) {
    kprintf("\n=== EXCEPTION %d ===\n", r->int_no);
    while (1) asm volatile ("hlt");
}


void irq_handler(struct regs* r) {
    int irq = r->int_no - 32;

    
    if (irq >= 8) outb(PIC2_COMMAND, 0x20);
    outb(PIC1_COMMAND, 0x20);

    if (irq == 0) {
        scheduler_tick();
    }
    if (irq == 1) {
        keyboard_handler();
    }
}
void syscall_init(void) {
    print_serial("Syscalls: int 0x80 ready\r\n");
}
void interrupts_init(void) {
    print_serial("Initializing interrupts...\r\n");

    pic_remap();

    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);

    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);
    extern void syscall_stub(void);
    idt_set_gate(0x80, (uint32_t)syscall_stub, 0x08, 0x8E);
    load_idt();
    timer_init(100);

    print_serial("Interrupts initialized (timer 100Hz)\r\n");
}
