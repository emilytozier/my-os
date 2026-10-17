#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

#define MAX_PROCESSES     64
#define KERNEL_STACK_SIZE 4096

typedef enum {
    PROCESS_NEW,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} ProcessState;

typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cr3;
} ProcessContext;

typedef struct Process {
    int pid;
    char name[32];
    ProcessState state;
    ProcessContext context;
    struct Process* next;
    int priority;
    uint32_t time_slice;
    uint32_t time_used;
    uint32_t stack_base;
    uint32_t stack_size;
    int exit_code;
    struct Process* parent;
} Process;

extern Process* process_table[MAX_PROCESSES];
extern Process* ready_queue;
extern Process* current_process;
extern Process* idle_process;
extern int next_pid;
extern Process* blocked_queue;
int process_sleep_pid(int pid, uint32_t ticks);
void process_wakeup(void);

void process_init(void);
Process* process_create_kernel(const char* name, void* entry_point);
void enqueue_ready(Process* proc);
void scheduler_tick(void);
void print_process_list(void);
void process_switch(Process* new_process);

// Время
void timer_set_frequency(uint32_t hz);
uint32_t timer_get_ticks(void);
uint32_t uptime_ms(void);

void process_sleep_ms(uint32_t ms);
#endif
