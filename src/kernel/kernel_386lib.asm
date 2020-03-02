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
; 导入头文件
%include "asm_const.inc"
; 导入变量
extern display_position         ; 当前显存的显示位置

; 导出函数
global low_print                ; 低特权级打印函数，只能支持打印ASCII码
global phys_copy                ; 通过物理地址拷贝内存
global in_byte                  ; 从一个端口读取一字节数据
global out_byte                 ; 向一个端口输出一字节数据
global in_word                  ; 从一个端口读取一字数据
global out_word                 ; 向一个端口输出一字数据
global interrupt_lock           ; 关闭中断响应，即锁中断
global interrupt_unlock         ; 打开中断响应，即解锁中断
global disable_irq              ; 屏蔽一个特定的中断
global enable_irq               ; 启用一个特定的中断
;*===========================================================================*
;*				phys_copy				     *
;*===========================================================================*
; PUBLIC void phys_copy(phys_bytes src, phys_bytes dest,
;			phys_bytes size);
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
    mov esi, [esp + PC_ARGS]            ; src
    mov edi, [esp + PC_ARGS + 4]        ; dest
    mov ecx, [esp + PC_ARGS + 4 + 4]    ; size
    ; 注：因为得到的就是物理地址，所以esi和edi无需再转换，直接就表示一个真实的位置。
phys_copy_start:
    cmp ecx, 0              ; 判断size
    jz phys_copy_end        ; if( size == 0 ); jmp phys_copy_end
    mov al, [esi]
    inc esi
    mov byte [edi], al
    inc edi
    dec ecx                 ; size--
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
;============================================================================
;   从一个端口读取一字节数据
; 函数原型： u8_t in_byte(port_t port)
;----------------------------------------------------------------------------
align 16
in_byte:
    push edx
        mov edx, [esp + 4]      ; 得到端口号
        xor eax, eax
        in al, dx              ; port -> al
    pop edx
    nop                         ; 一点延迟
    ret
;============================================================================
;   向一个端口输出一字节数据
; 函数原型： void out_byte(port_t port, U8_t value)
;----------------------------------------------------------------------------
align 16
out_byte:
    push edx
        mov edx, [esp + 4]      ; 得到端口号
        mov al, [esp + 4 * 2]   ; 要输出的字节
        out dx, al              ; al -> port
    pop edx
    nop                         ; 一点延迟
    ret
;============================================================================
;   从一个端口读取一字数据
; 函数原型： u16_t in_word(port_t port)
;----------------------------------------------------------------------------
align 16
in_word:
    push edx
        mov edx, [esp + 4]      ; 得到端口号
        xor eax, eax
        in ax, dx              ; port -> ax
    pop edx
    nop                         ; 一点延迟
    ret
;============================================================================
;   向一个端口输出一字数据
; 函数原型： void out_word(port_t port, U16_t value)
;----------------------------------------------------------------------------
align 16
out_word:
    push edx
        mov edx, [esp + 4]      ; 得到端口号
        mov ax, [esp + 4 * 2]   ; 得到要输出的变量
        out dx, ax              ; ax -> port
    pop edx
    nop                         ; 一点延迟
    ret
;============================================================================
;   关闭中断响应，也称为锁中断
; 函数原型： void interrupt_lock(void)
;----------------------------------------------------------------------------
align 16
interrupt_lock:
        cli
    ret
;============================================================================
;   打开中断响应，也称为解锁中断
; 函数原型： void interrupt_unlock(void)
;----------------------------------------------------------------------------
align 16
interrupt_unlock:
        sti
    ret
;============================================================================
;   屏蔽一个特定的中断
; 函数原型： int disable_irq(int int_request);
;----------------------------------------------------------------------------
align 16
disable_irq:
    pushf                   ; 将标志寄存器 EFLAGS 压入堆栈，需要用到test指令，会改变 EFLAGS
    push ecx

        cli                     ; 先屏蔽所有中断
        mov ecx, [esp + 4]      ; ecx = int_request(中断向量)
        ; 判断要关闭的中断来自于哪个 8259A
        mov ah, 1               ; ah = 00000001b
        rol ah, cl              ; ah = (1 << (int_request % 8))，算出在int_request位的置位位图，例如2的置位位图是00000100b
        cmp cl, 7
        ja disable_slave        ; 0~7主，8~15从；> 7是从，跳转到 disable_slave 处理 从8259A 的中断关闭
disable_master:                 ; <= 7是主
        in al, INT_M_CTLMASK    ; 取出 主8259A 当前的屏蔽位图
        test al, ah
        jnz disable_already     ; 该int_request的屏蔽位图不为0，说明已经被屏蔽了，没必要继续了
        ; 该int_request的屏蔽位图为0，还未被屏蔽
        or al, ah               ; 将该中断的屏蔽位置位，表示屏蔽它
        out INT_M_CTLMASK, al   ; 输出新的屏蔽位图，屏蔽该中断
        jmp disable_ok          ; 屏蔽完成
disable_slave:
        in al, INT_S_CTLMASK    ; 取出 从8259A 当前的屏蔽位图
        test al, ah
        jnz disable_already     ; 该int_request的屏蔽位图不为0，说明已经被屏蔽了，没必要继续了
        ; 该int_request的屏蔽位图为0，还未被屏蔽
        or al, ah               ; 将该中断的屏蔽位置位，表示屏蔽它
        out INT_S_CTLMASK, al   ; 输出新的屏蔽位图，屏蔽该中断
disable_ok:
    pop ecx
    popf
    and eax, 1              ; 等同于 mov eax, 1，即return 1；我只是想耍个帅！
    ret
disable_already:
    pop ecx
    popf                    ; 恢复标志寄存器
    xor eax, eax            ; return 0，表示屏蔽失败，因为该中断已经处于屏蔽状态
    ret
;============================================================================
;   启用一个特定的中断
; 函数原型： void enable_irq(int int_request);
;----------------------------------------------------------------------------
align 16
enable_irq:
    pushf                   ; 将标志寄存器 EFLAGS 压入堆栈，需要用到test指令，会改变 EFLAGS
    push ecx

        cli                     ; 先屏蔽所有中断
        mov ecx, [esp + 4]      ; ecx = int_request(中断向量)
        mov ah, ~1              ; ah = 11111110b
        rol ah, cl              ; ah = ~(1 << (int_request % 8))，算出在int_request位的复位位位图，例如2的置位位图是11111011b
        cmp cl, 7
        ja enable_slave         ; 0~7主，8~15从；> 7是从，跳转到 disable_slave 处理 从8259A 的中断关闭
enable_master:                  ; <= 7是主
        in al, INT_M_CTLMASK    ; 取出 主8259A 当前的屏蔽位图
        and al, ah              ; 将该中断的屏蔽位复位，表示启用它
        out INT_M_CTLMASK, al   ; 输出新的屏蔽位图，启用该中断
        jmp enable_ok
enable_slave:
        in al, INT_S_CTLMASK    ; 取出 从8259A 当前的屏蔽位图
        and al, ah              ; 将该中断的屏蔽位复位，表示启用它
        out INT_S_CTLMASK, al   ; 输出新的屏蔽位图，启用该中断
enable_ok:
      pop ecx
      popf
      ret
