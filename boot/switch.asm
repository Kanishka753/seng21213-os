[BITS 32]

global context_switch

; context_switch(uint32_t *old_esp, uint32_t new_esp)
;
; [esp+4] = pointer to old ESP
; [esp+8] = new ESP

context_switch:
    pushad

    mov eax, [esp + 36]
    mov [eax], esp

    mov esp, [esp + 40]

    popad
    ret
