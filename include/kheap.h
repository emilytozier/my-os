#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>

void kheap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

// Для отладки
size_t kheap_used(void);
size_t kheap_free(void);

#endif
