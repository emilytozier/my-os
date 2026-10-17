#include "keyboard.h"
#include "screen.h"
#include "serial.h"
#include "shell.h"

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static const char keymap[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,  '\\','z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char keymap_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static int shift_pressed = 0;
static int caps_lock = 0;
static int extended_mode = 0;

void keyboard_init(void) {
    print_serial("Keyboard: IRQ1 driver ready\r\n");
}


void keyboard_handler(void) {
    unsigned char status = inb(0x64);
    if (!(status & 0x01)) return;

    int scancode = inb(0x60);
  
    if (scancode == 0xE0) {
        extended_mode = 1;
        return;
    }

    if (scancode < 128) {       
        if (extended_mode) {
            extended_mode = 0;
            switch (scancode) {
                case 0x48: shell_process_input(0x80); break; // Up
                case 0x50: shell_process_input(0x81); break; // Down
                case 0x4B: shell_process_input(0x82); break; // Left
                case 0x4D: shell_process_input(0x83); break; // Right
                case 0x47: shell_process_input(0x84); break; // Home
                case 0x4F: shell_process_input(0x85); break; // End
            }
            return;
        }

        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
            return;
        }
        if (scancode == 0x3A) {
            caps_lock = !caps_lock;
            return;
        }

        char c = 0;
        if (shift_pressed) {  
            if (caps_lock && scancode >= 0x10 && scancode <= 0x32) {
                c = keymap[scancode];  
            } else {
                c = keymap_shift[scancode];
            }
        } else {
            if (caps_lock) {
                c = keymap_shift[scancode];
            } else {
                c = keymap[scancode];
            }
        }

        if (c != 0) {
            shell_process_input(c);
        }
    } else {
        int release = scancode - 128;
        if (release == 0x2A || release == 0x36) {
            shift_pressed = 0;
        }
        if (extended_mode) {
            extended_mode = 0;
        }
    }
}
