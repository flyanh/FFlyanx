;================================================================================================
; 文件：kernel_386_lib.asm
; 作用：Flyanx系统内核内核库文件
; 作者： Flyan
; 联系QQ： 1341662010
; QQ-Group:909830414
; gitee: https://gitee.com/flyanh/
;================================================================================================

;============================================================================
;   导入和导出
;----------------------------------------------------------------------------
; 导入变量
extern display_position

; 导出函数
global low_print
global phys_copy
;*===========================================================================*
;*				phys_copy				     *
;*===========================================================================*
; PUBLIC void phys_copy(phys_bytes source, phys_bytes destination,
;			phys_bytes bytecount);
;* 将物理内存中任意处的一个数据块拷贝到任意的另外一处 *
;* 参数中的两个地址都是绝对地址，也就是地址0确实表示整个地址空间的第一个字节， *
;* 并且三个参数均为无符号长整数 *
PC_ARGS     equ     16    ; 用于到达复制的参数堆栈的栈顶
align 16
phys_copy:
    push esi
    push edi
    push es

    ; 获得所有参数
    mov esi, [esp + PC_ARGS]            ; source
    mov edi, [esp + PC_ARGS + 4]        ; destination
    mov ecx, [esp + PC_ARGS + 4 + 4]    ; bytecoun
    ; 注：因为得到的就是物理地址，所以esi和edi无需再转换，直接就表示一个真实的位置。
phys_copy_start:
    cmp ecx, 0              ; 判断bytecount
    jz phys_copy_end        ; if( bytecount == 0 ); jmp phys_copy_end
    mov al, [esi]
    inc esi
    mov byte [edi], al
    inc edi
    dec ecx                 ; bytecount--
    jmp phys_copy_start
phys_copy_end:
    pop es
    pop edi
    pop esi
    ret
;============================================================================
;   打印函数，它类似与C语言中的printf，但它不支持'%'可变参数
; 函数原型：void low_print(char* str)，字符串以0结尾
;----------------------------------------------------------------------------
align 16
low_print:
    push esi
    push edi

    mov esi, [esp + 4 * 3]      ; 得到字符串地址
    mov edi, [display_position]   ; 得到显示位置
    mov ah, 0xf                 ; 黑底白字
.1:
    lodsb                       ; ds:esi -> al, esi++
    test al, al
    jz .PrintEnd                ; 遇到了0，结束打印
    cmp al, 10
    je .2
    ; 如果不是0，也不是'\n'，那么我们认为它是一个可打印的普通字符
    mov [gs:edi], ax
    add edi, 2                  ; 指向下一列
    jmp .1
.2: ; 处理换行符'\n'
    push eax
    mov eax, edi                ; eax = 显示位置
    mov bl, 160
    div bl                      ; 显示位置 / 160，商eax就是当前所在行数
    inc eax                     ; 行数++
    mov bl, 160
    mul bl                      ; 行数 * 160，得出这行的显示位置
    mov edi, eax                ; edi = 新的显示位置
    pop eax
    jmp .1
.PrintEnd:
    mov dword [display_position], edi ; 打印完毕，更新显示位置

    pop edi
    pop esi
    ret
