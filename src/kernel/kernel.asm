;================================================================================================
; 文件：kernel.asm
; 作用：Flyanx系统内核文件
; 作者： Flyan
; QQ: 1341662010
; QQ-Group:909830414
; gitee: https://gitee.com/flyanh/
;================================================================================================

;============================================================================
;   导入和导出
;----------------------------------------------------------------------------
; 导入头文件
%include "asm_const.inc"
; 导入函数
extern cstart                       ; 初始化一些事情，主要是改变gdt_ptr，让它指向新的GDT
extern flyanx_main                  ; 内核主函数

; 导入变量
extern gdt_ptr                      ; GDT指针
extern idt_ptr                      ; IDT指针
extern tss                          ; 任务状态段

; 导出函数
global _start                       ; 导出_start程序开始符号，链接器需要它
;============================================================================
;   内核数据段
;----------------------------------------------------------------------------
[section .data]
bits 32
    nop
;============================================================================
;   内核堆栈段
;----------------------------------------------------------------------------
[section .bss]
StackSpace:     resb 4 * 1024       ; 4KB栈空间
StackTop:
;============================================================================
;   内核代码段
;----------------------------------------------------------------------------
[section .text]
_start:     ; 内核程序入口
    ; 注意! 在使用 C 代码的时候一定要保证 ds, es, ss 这几个段寄存器的值是一样的
    ; 因为编译器有可能编译出使用它们的代码, 而编译器默认它们是一样的. 比如串拷贝操作会用到 ds 和 es。
    ; 寄存器复位
    mov ax, ds
    mov es, ax
    mov fs, ax
    mov ss, ax              ; es = fs = ss = 内核数据段
    mov esp, StackTop       ; 栈顶

    ; 调用cstart函数做一些初始化工作，其中就包括改变gdt_ptr，让它指向新的GDT
    sgdt [gdt_ptr]          ; 将GDT指针保存到gdt_ptr中
    call cstart
    lgdt [gdt_ptr]          ; 使用新的GDT
    lidt [idt_ptr]          ; 加载idt指针，在cstart函数中已经将idt_ptr指向新的中断表了

    ; 一个跳转指令，让以上设置及时生效
    jmp SELECTOR_KERNEL_CS:csinit

csinit:
    ; 加载任务状态段 TSS
    xor eax, eax
    mov ax, SELECTOR_TSS
    ltr ax
    ; 跳入C语言编写的主函数，在这之后我们内核的开发工作主要用C开发了
    ; 这一步，我们迎来了质的飞跃，汇编虽然好，只是不够骚！
    jmp flyanx_main

    ; 永远不可能到的真实
    jmp $

