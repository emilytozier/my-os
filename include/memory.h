#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>


#define KERNEL_START 0x100000 
#define STACK_SIZE   16384       
#define HEAP_SIZE    (1024 * 1024) 

void memory_init(void);


unsigned int memory_used(void);  
unsigned int memory_free(void);  
size_t get_heap_used(void);    

#endif
