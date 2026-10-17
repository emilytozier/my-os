#include "process.h"
#include "kheap.h"
#include "string.h"
#include "screen.h"
#include "serial.h"

Process* process_table[MAX_PROCESSES];
Process* ready_queue = NULL;
Process* blocked_queue = NULL;
Process* current_process = NULL;
Process* idle_process = NULL;
int next_pid = 1;
static uint32_t timer_ticks = 0;
static uint32_t timer_hz = 100;

extern void switch_context(uint32_t* old_esp_ptr, uint32_t new_esp);

static uint32_t prepare_stack(uint32_t stack_top, void* entry) {
    uint32_t* sp = (uint32_t*)stack_top;

    *(--sp) = (uint32_t)entry;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;

    return (uint32_t)sp;
}

static void idle_func(void) {
    while (1) {
        asm volatile ("hlt");
    }
}


Process* process_create_kernel(const char* name, void* entry_point) {
    int pid = next_pid++;
    if (pid >= MAX_PROCESSES) return NULL;

    Process* proc = (Process*)kmalloc(sizeof(Process));
    if (!proc) return NULL;

    memset(proc, 0, sizeof(Process));
    proc->pid = pid;
    strncpy(proc->name, name, 31);
    proc->name[31] = '\0';
    proc->state = PROCESS_NEW;
    proc->priority = 5;
    proc->time_slice = 20;
    proc->time_used = 0;
    proc->next = NULL;

    proc->stack_base = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    if (!proc->stack_base) {
        kfree(proc);
        return NULL;
    }
    proc->stack_size = KERNEL_STACK_SIZE;

    void* real_entry = (entry_point != NULL) ? entry_point : (void*)idle_func;

    uint32_t top = proc->stack_base + KERNEL_STACK_SIZE;
    proc->context.esp = prepare_stack(top, real_entry);
    proc->context.eip = (uint32_t)real_entry;

    process_table[pid] = proc;

    print_serial("Created process: ");
    print_serial(name);
    print_serial("\r\n");

    return proc;
}

void enqueue_ready(Process* proc) {
    proc->state = PROCESS_READY;
    proc->next = NULL;

    if (ready_queue == NULL) {
        ready_queue = proc;
    } else {
        Process* last = ready_queue;
        while (last->next) last = last->next;
        last->next = proc;
    }
}

static Process* dequeue_ready(void) {
    if (!ready_queue) return NULL;
    Process* proc = ready_queue;
    ready_queue = ready_queue->next;
    proc->next = NULL;
    return proc;
}

void remove_from_ready_queue(Process* proc) {
    Process* prev = NULL;
    Process* cur = ready_queue;
    while (cur) {
        if (cur == proc) {
            if (prev) prev->next = cur->next;
            else ready_queue = cur->next;
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

void process_switch(Process* new_process) {
    if (current_process == new_process) return;
    if (!new_process) return;

    Process* old = current_process;

    old->state = PROCESS_READY;

    new_process->state = PROCESS_RUNNING;
    new_process->time_used = 0;
    current_process = new_process;

    switch_context(&old->context.esp, new_process->context.esp);
}

void process_init(void) {
    print_serial("Initializing process manager...\r\n");

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i] = NULL;
    }

    idle_process = process_create_kernel("idle", NULL);
    if (idle_process) {
        idle_process->state = PROCESS_RUNNING;
        current_process = idle_process;
    }

    print_serial("Process manager initialized\r\n");
}

void process_sleep(uint32_t ticks) {
    if (current_process == NULL) return;
    if (current_process == idle_process) return;
    if (ticks == 0) return;
    
    print_serial("Sleep: ");
    print_serial(current_process->name);
    print_serial(" for ");
    print_serial("...");
    print_serial(" ticks\r\n");
    
    Process* old = current_process;
    old->state = PROCESS_BLOCKED;
    old->time_used = ticks;  
    
    old->next = blocked_queue;
    blocked_queue = old;
 
    Process* next = dequeue_ready();
    if (next == NULL) next = idle_process;
    
    current_process = next;
    next->state = PROCESS_RUNNING;
    next->time_used = 0;
    
    switch_context(&old->context.esp, next->context.esp);
}



int process_sleep_pid(int pid, uint32_t ticks) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;
    
    Process* p = process_table[pid];
    if (p == NULL) return -2;
    if (p == idle_process) return -3;
    if (p == current_process) return -4;
    if (ticks == 0) return -5;
    
    remove_from_ready_queue(p);
    
    p->state = PROCESS_BLOCKED;
    p->time_used = ticks;
    p->next = blocked_queue;
    blocked_queue = p;
    
    print_serial("Sleeping pid ");
    print_serial(p->name);
    print_serial("\r\n");
    
    return 0;
}
void process_wakeup(void) {
    Process* prev = NULL;
    Process* cur = blocked_queue;
    
    while (cur) {
        if (cur->time_used > 0) {
            cur->time_used--;
        }

        if (cur->time_used == 0) {        
            Process* next = cur->next;

            if (prev) prev->next = next;
            else blocked_queue = next;

            enqueue_ready(cur);
            cur = next;
        } else {
            prev = cur;
            cur = cur->next;
        }
    }
}


void scheduler_tick(void) {
    if (current_process == NULL) return;
    timer_ticks++; 
    process_wakeup();   
    current_process->time_used++;

    static int tick_count = 0;
    tick_count++;
    if (tick_count % 100 == 0) {
        print_serial("tick 1s: current = ");
        print_serial(current_process->name);
        print_serial("\r\n");
    }

    if (current_process->time_used < current_process->time_slice) {
        return;
    }

    Process* old = current_process;
    old->time_used = 0;

    enqueue_ready(old);

    Process* next = dequeue_ready();

    if (next == old) {
        return;
    }

    if (next == NULL) {
        return;
    }

    print_serial("switch: ");
    print_serial(old->name);
    print_serial(" -> ");
    print_serial(next->name);
    print_serial("\r\n");

    process_switch(next);
}


void process_exit(int exit_code) {
    if (current_process == NULL) return;
    if (current_process == idle_process) return;

    Process* p = current_process;

    print_serial("Process ");
    print_serial(p->name);
    print_serial(" exited\r\n");

    p->state = PROCESS_TERMINATED;
    p->exit_code = exit_code;

    if (p->stack_base != 0) {
        kfree((void*)p->stack_base);
        p->stack_base = 0;
    }

    process_table[p->pid] = NULL;

    Process* next = dequeue_ready();
    if (next == NULL) {
        next = idle_process;
    }

    current_process = next;
    next->state = PROCESS_RUNNING;
    next->time_used = 0;

    switch_context(&p->context.esp, next->context.esp);
}

int process_kill(int pid) {
    if (pid < 0 || pid >= MAX_PROCESSES) return -1;

    Process* p = process_table[pid];
    if (p == NULL) return -2;
    if (p == idle_process) return -3;
    if (p == current_process) return -4;

    print_serial("Killing process ");
    print_serial(p->name);
    print_serial("\r\n");

    p->state = PROCESS_TERMINATED;
    p->exit_code = -1;

    remove_from_ready_queue(p);

    if (p->stack_base != 0) {
        kfree((void*)p->stack_base);
        p->stack_base = 0;
    }

 
    process_table[p->pid] = NULL;

    return 0;
}

uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

uint32_t uptime_ms(void) {
    return (timer_ticks * 1000) / timer_hz;
}

void timer_set_frequency(uint32_t hz) {
    if (hz > 0) timer_hz = hz;
}

void process_sleep_ms(uint32_t ms) {
    if (timer_hz == 0) return;
    uint32_t ticks = (ms * timer_hz) / 1000;
    if (ticks == 0) ticks = 1;   
    process_sleep(ticks);
}


void print_process_list(void) {
    kprintf("\n");
    kprintf("PID  ");
    kprintf("Name          ");
    kprintf("State         ");
    kprintf("Time\n");
    kprintf("---  ");
    kprintf("----          ");
    kprintf("-----         ");
    kprintf("----\n");

    for (int i = 0; i < MAX_PROCESSES; i++) {
        Process* p = process_table[i];
        if (!p) continue;

        const char* s;
        switch (p->state) {
            case PROCESS_NEW:        s = "NEW"; break;
            case PROCESS_READY:      s = "READY"; break;
            case PROCESS_RUNNING:    s = "RUNNING"; break;
            case PROCESS_BLOCKED:    s = "BLOCKED"; break;
            case PROCESS_TERMINATED: s = "TERMINATED"; break;
            default:                 s = "?"; break;
        }

        kprintf("%d", p->pid);
        int pid_len = 1;
        if (p->pid >= 10)  pid_len = 2;
        if (p->pid >= 100) pid_len = 3;
        for (int k = pid_len; k < 5; k++) kprintf(" ");

        int nlen = strlen(p->name);
        kprintf("%s", p->name);
        for (int k = nlen; k < 14; k++) kprintf(" ");

        int slen = strlen(s);
        kprintf("%s", s);
        for (int k = slen; k < 14; k++) kprintf(" ");

        kprintf("%d\n", p->time_used);
    }
}
