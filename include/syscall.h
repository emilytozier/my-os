#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYS_WRITE     1
#define SYS_EXIT      2
#define SYS_SLEEP     3
#define SYS_GETPID    4
#define SYS_UPTIME    5
#define SYS_PUTCHAR   6

void syscall_init(void);

void syscall_handler(uint32_t* regs);

#endif
