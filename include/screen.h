
#ifndef SCREEN_H
#define SCREEN_H

int get_cursor_y(void);
void print_char_at(char c, int x, int y);
void clear_screen(void);
void print_char(char c);
void print_string(const char* str);
void kprintf(const char* format, ...);
void set_color(unsigned char foreground, unsigned char background);

#endif
