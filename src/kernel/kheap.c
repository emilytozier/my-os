#include "kheap.h"
#include "string.h"

#define HEAP_SIZE  (1024 * 1024)
#define ALIGN      8

typedef struct Block {
    size_t size;       
    int    free;        
    struct Block* next;   
    struct Block* prev;    
} Block;

static unsigned char heap_pool[HEAP_SIZE] __attribute__((aligned(16)));
static Block* head = NULL;
static size_t used_bytes = 0;

#define HEADER_SIZE  (sizeof(Block))
#define MIN_SPLIT    (HEADER_SIZE + ALIGN)

void kheap_init(void) {
    head = (Block*)heap_pool;
    head->size = HEAP_SIZE - HEADER_SIZE;
    head->free = 1;
    head->next = NULL;
    head->prev = NULL;
    used_bytes = 0;
}

static size_t align_up(size_t n) {
    return (n + (ALIGN - 1)) & ~(size_t)(ALIGN - 1);
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = align_up(size);

    Block* cur = head;
    while (cur) {
        if (cur->free && cur->size >= size) {          
            if (cur->size >= size + MIN_SPLIT) {
                Block* nb = (Block*)((unsigned char*)cur + HEADER_SIZE + size);
                nb->size = cur->size - size - HEADER_SIZE;
                nb->free = 1;
                nb->next = cur->next;
                nb->prev = cur;
                if (cur->next) cur->next->prev = nb;

                cur->size = size;
                cur->next = nb;
            }
            cur->free = 0;
            used_bytes += cur->size + HEADER_SIZE;
            return (void*)((unsigned char*)cur + HEADER_SIZE);
        }
        cur = cur->next;
    }
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;

    Block* b = (Block*)((unsigned char*)ptr - HEADER_SIZE);
    if (b->free) return;

    b->free = 1;
    used_bytes -= b->size + HEADER_SIZE;

 
    if (b->next && b->next->free) {
        Block* nb = b->next;
        b->size += HEADER_SIZE + nb->size;
        b->next = nb->next;
        if (nb->next) nb->next->prev = b;
    }
 
    if (b->prev && b->prev->free) {
        Block* pb = b->prev;
        pb->size += HEADER_SIZE + b->size;
        pb->next = b->next;
        if (b->next) b->next->prev = pb;
    }
}

size_t kheap_used(void) { return used_bytes; }
size_t kheap_free(void) { return HEAP_SIZE - used_bytes; }
