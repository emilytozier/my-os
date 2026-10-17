[BITS 32]

global switch_context

; void switch_context(uint32_t* old_esp_ptr, uint32_t new_esp)
;
; Соглашение вызова cdecl: аргументы в стеке.
; При входе: [esp+0]=ret, [esp+4]=old_esp_ptr, [esp+8]=new_esp
switch_context:  
    mov eax, [esp + 4]        ; eax = old_esp_ptr
    mov edx, [esp + 8]        ; edx = new_esp

    push ebp
    push ebx
    push esi
    push edi

    
    mov [eax], esp

    mov esp, edx

    pop edi
    pop esi
    pop ebx
    pop ebp

    ; ret прыгнет на адрес из вершины нового стека
    ret
