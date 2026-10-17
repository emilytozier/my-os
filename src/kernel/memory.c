
#include "memory.h"
#include "screen.h"
#include "kheap.h"

void memory_init(void) {
    kheap_init();
    kprintf("Memory manager initialized\n");
    kprintf("Heap size: %d bytes\n", HEAP_SIZE);
}
unsigned int memory_used(void) {
    return (unsigned int)kheap_used() + STACK_SIZE;
}

unsigned int memory_free(void) {
    return (unsigned int)kheap_free();
}
