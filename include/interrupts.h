#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdint.h>

#define IDT_ENTRIES 256

struct regs {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void interrupts_init(void);
void idt_set_gate(int num, uint32_t base, uint16_t sel, uint8_t flags);
void isr_handler(struct regs *r);
void irq_handler(struct regs *r);

extern void isr0(void);
extern void isr1(void);
extern void isr6(void);
extern void isr8(void);
extern void isr13(void);
extern void isr14(void);

extern void irq0(void);
extern void irq1(void);

#endif
