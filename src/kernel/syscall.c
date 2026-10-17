#include "syscall.h"
#include "screen.h"
#include "serial.h"
#include "process.h"


void syscall_handler(uint32_t* regs) {
    uint32_t num = regs[0];   
    uint32_t a1  = regs[1]; 
    uint32_t a2  = regs[2];   
    uint32_t a3  = regs[3]; 

    uint32_t result = 0;

    switch (num) {
        case SYS_WRITE: {
            const char* s = (const char*)a1;
            if (s) {
                kprintf("%s", s);
            }
            result = 0;
            break;
        }

        case SYS_PUTCHAR:
            print_char((char)a1);
            result = 0;
            break;

        case SYS_EXIT:
            print_serial("sys_exit called\r\n");
            process_exit((int)a1);
            
            break;

        case SYS_SLEEP:
            process_sleep_ms(a1);
            result = 0;
            break;

        case SYS_GETPID:
            if (current_process) {
                result = (uint32_t)current_process->pid;
            }
            break;

        case SYS_UPTIME:
            result = uptime_ms();
            break;

        default:
            kprintf("syscall: unknown %d\n", (int)num);
            result = (uint32_t)-1;
            break;
    }

    regs[0] = result;   
}
