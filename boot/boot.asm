[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Save boot drive
    mov [boot_drive], dl

    ; Collect BIOS E820 memory map
    xor ebx, ebx
    mov di, 0x8000
    xor bp, bp

e820_loop:
    mov eax, 0xE820
    mov edx, 0x534D4150
    mov ecx, 24
    int 0x15

    jc e820_done
    cmp eax, 0x534D4150
    jne e820_done

    inc bp
    add di, 24

    test ebx, ebx
    jnz e820_loop

e820_done:
    mov [e820_count], bp

    ; Load kernel sectors to physical address 0x10000
    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    mov ah, 0x02
    mov al, 64
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; Enter protected mode
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode

disk_error:
    hlt
    jmp disk_error

[BITS 32]

protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov esp, 0x90000

    ; Pass E820 map address and count
    mov eax, 0x8000
    mov ebx, [e820_count]

    call 0x10000

halt:
    cli
    hlt
    jmp halt

boot_drive db 0
e820_count dw 0

gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF

gdt_descriptor:
    dw gdt_descriptor - gdt_start - 1
    dd gdt_start

times 510-($-$$) db 0
dw 0xAA55
