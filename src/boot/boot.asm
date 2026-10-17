section .multiboot_header
align 4

multiboot_header:
    dd 0x1BADB002              ; magic number
    dd 0x00000003              ; flags
    dd -(0x1BADB002 + 0x00000003) ; checksum

section .text
global _start
extern kernel_main

_start:  
    cmp eax, 0x2BADB002
    jne .no_multiboot
    
    ; Настройка стека
    mov esp, stack_top
    
    push ebx               
    
    call kernel_main
   
    jmp .halt

.no_multiboot:  
    mov dword [0xB8000], 0x4F524F45  ;
    jmp .halt

.halt:
    cli
    hlt
    jmp .halt

section .bss
align 16
stack_bottom:
    resb 16384                 ; 16KB стека
stack_top:
