// src/kernel/kernel.c
#include "screen.h"
#include "keyboard.h"
#include "string.h"
#include "serial.h"
#include "shell.h"
#include "kheap.h"
#include "fs.h"
#include "syscall.h"
#include "interrupts.h"
#include "process.h"

void dummy_process(void) {
    asm volatile ("sti");
    while (1) {
        for (volatile int i = 0; i < 3000000; i++);
    }
}

void syscall_test_process(void) {
    asm volatile ("sti");

    uint32_t pid;
    asm volatile (
        "mov $4, %%eax\n"      
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=r"(pid)
        :
        : "eax"
    );

    const char* msg = "syscall_test: hello from userspace!\n";
    asm volatile (
       "int $0x80\n"
       :
       : "a"(1), "b"(msg)  
       : "memory"
    );
    // Спать 1 секунду
    asm volatile (
        "mov $3, %%eax\n"
        "mov $1000, %%ebx\n"
        "int $0x80\n"
        :
        :
        : "eax", "ebx"
    );


    uint32_t up;
    asm volatile (
        "mov $5, %%eax\n"  
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=r"(up)
        :
        : "eax"
    );

    const char* msg2 = "syscall_test: uptime was queried\n";
    asm volatile (
        "mov $1, %%eax\n"
        "mov %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r"(msg2)
        : "eax", "ebx"
    );

    while (1) {
        asm volatile (
            "mov $3, %%eax\n"
            "mov $2000, %%ebx\n"
            "int $0x80\n"
            :
            :
            : "eax", "ebx"
        );
    }
}

void kernel_main(unsigned int multiboot_info_ptr) {
    (void)multiboot_info_ptr;

    serial_init();
    print_serial("=== MyOS Microkernel ===\r\n");

    clear_screen();
    set_color(0x0F, 0x00);

    kheap_init();
    interrupts_init();
    syscall_init();
    keyboard_init();
    process_init();
    
    if (idle_process) {
        idle_process->time_slice = 50;
    }
    Process* st = process_create_kernel("syscall_test", syscall_test_process);
    if (st) {
       st->time_slice = 20;
       enqueue_ready(st);
    }
    fs_init();
    shell_init();

    asm volatile ("sti");

    shell_run();

    while (1) {
        asm volatile ("hlt");
    }
}
