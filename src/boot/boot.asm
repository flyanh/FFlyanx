org 0x7c00

    jmp START
    nop

StackBase   equ     0x7c00

; db 定义一个字节  dw 字word  dd 定义一个双字double word
BootMessage:    db "Booting......"

START:
    mov ax, cs
    mov ds, ax
    mov ss, ax
    mov sp, StackBase

    ; 打印字符串"Booting..."
    mov al, 1
    mov bh, 0
    mov bl, 0x07        ; 黑底白字
    mov cx, 13
    mov dh, 0
    mov dh, 0
    ; es = ds
    push ds
    pop es
    mov bp, BootMessage
    mov ah, 0x13
    int 0x10

    jmp $


; times n m        n：重复定义多少次   m:定义的数据
times 510-($-$$)   db    0
dw  0xaa55          ; 引导扇区标志










