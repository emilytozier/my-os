#ifndef SERIAL_H
#define SERIAL_H

void serial_init(void);

void print_serial_char(char c);

void print_serial(const char* str);

char read_serial(void);

void serial_write(char c);

#endif
