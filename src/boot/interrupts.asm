section .text


global load_idt
load_idt:
    mov eax, [esp + 4]  
    lidt [eax]          
    ret

global isr0
isr0:
    cli
    push byte 0
    push byte 0
    jmp isr_common_stub

global isr6
isr6:
    cli
    push byte 0
    push byte 6
    jmp isr_common_stub

global isr8
isr8:
    cli
    push byte 8
    jmp isr_common_stub

global isr13
isr13:
    cli
    push byte 13
    jmp isr_common_stub

global isr14
isr14:
    cli
    push byte 14
    jmp isr_common_stub

global irq1
irq1:
    cli
    push byte 0
    push byte 33        ; IRQ1 -> INT 33
    jmp irq_common_stub

isr_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp
    extern isr_handler
    call isr_handler
    pop esp
    
    pop gs
    pop fs
    pop es
    pop ds
    popa
    
    add esp, 8
    iret


irq_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp
    extern irq_handler
    call irq_handler
    pop esp
    
    pop gs
    pop fs
    pop es
    pop ds
    popa
    
    add esp, 8
    iret
