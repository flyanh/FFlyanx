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
extern exception_handler            ; 异常统一处理例程

; 导入变量
extern gdt_ptr                      ; GDT指针
extern idt_ptr                      ; IDT指针
extern tss                          ; 任务状态段
extern irq_handler_table            ; 硬件中断请求处理例程表

; 导出函数
global _start                       ; 导出_start程序开始符号，链接器需要它
global down_run                     ; 系统进入宕机
; 所有的异常处理入口
global divide_error
global single_step_exception
global nmi
global breakpoint_exception
global overflow
global bounds_check
global inval_opcode
global copr_not_available
global double_fault
global copr_seg_overrun
global inval_tss
global segment_not_present
global stack_exception
global general_protection
global page_fault
global copr_error
; 所有中断处理入口，一共16个(两个8259A)
global	hwint00
global	hwint01
global	hwint02
global	hwint03
global	hwint04
global	hwint05
global	hwint06
global	hwint07
global	hwint08
global	hwint09
global	hwint10
global	hwint11
global	hwint12
global	hwint13
global	hwint14
global	hwint15
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

;    jmp 0x40:0		; 手动触发一个General Protection异常，测试异常机制
;    ud2             ; 手动触发一个无效指令异常

    ; 主动使用 int n 触发3、9 中断请求
    int 32 + 3
    int 32 + 9
    ; 主动使用 int 48 测试软件中断
    int 48

    ; 跳入C语言编写的主函数，在这之后我们内核的开发工作主要用C开发了
    ; 这一步，我们迎来了质的飞跃，汇编虽然好，只是不够骚！
    jmp flyanx_main

    ; 永远不可能到的真实
    jmp $
;============================================================================
;   硬件中断处理
;----------------------------------------------------------------------------
; 为 主从两个8259A 各定义一个中断处理模板
;----------------------------------------------------------------------------
%macro  hwint_master 1
    ; 1 在调用对于中断的处理例程前，先屏蔽当前中断，防止短时间内连续发生好几次同样的中断
    in al, INT_M_CTLMASK    ; 取出 主8259A 当前的屏蔽位图
    or al, (1 << %1)        ; 将该中断的屏蔽位置位，表示屏蔽它
    out INT_M_CTLMASK, al   ; 输出新的屏蔽位图，屏蔽该中断

    ; 2 重新启用 主8259A 和中断响应；CPU 响应一个中断后会自动关闭中断，同时 8259A 也会自动关闭自己
    ;   重新启用中断响应是为了及时的响应其他中断，很简单的道理：你在敲键盘的时候？就不允许磁盘中断响应操作磁盘了么？
    mov al, EOI
    out INT_M_CTL, al       ; 设置 EOI 位，重新启用 主8259A
    nop
    sti                     ; 重新启动中断响应

    ; 3 现在调用中断处理例程
    push %1                 ; 压入中断向量号作为参数
    call [irq_handler_table + (4 * %1)] ; 调用中断处理程序表中的相应处理例程，返回值存放在 eax 中
    add esp, 4              ; 清理堆栈

    ; 4 最后，判断用户的返回值，如果是DISABLE(0)，我们就直接结束；如果不为0，那么我们就重新启用当前中断
    cli                     ; 先将中断响应关闭，这个时候不允许其它中断的干扰
    cmp eax, DISABLE
    je .0                   ; 返回值 == DISABLE，直接结束中断处理
    ; 返回值 != DISABLE，重新启用当前中断
    in al, INT_M_CTLMASK    ; 取出 主8259A 当前的屏蔽位图
    and al, ~(1 << %1)      ; 将该中断的屏蔽位复位，表示启用它
    out INT_M_CTLMASK, al   ; 输出新的屏蔽位图，启用该中断
    ; 这里不需要使用 sti 指令重新启用中断响应，因为在输出新的屏蔽位图到 8259A 后，中断会重新开始响应
.0:
    ret
%endmacro
align	16
hwint00:		; Interrupt routine for irq 0 (the clock)，时钟中断
 	hwint_master	0

align	16
hwint01:		; Interrupt routine for irq 1 (keyboard)，键盘中断
 	hwint_master	1

align	16
hwint02:		; Interrupt routine for irq 2 (cascade!)
 	hwint_master	2

align	16
hwint03:		; Interrupt routine for irq 3 (second serial)
 	hwint_master	3

align	16
hwint04:		; Interrupt routine for irq 4 (first serial)
 	hwint_master	4

align	16
hwint05:		; Interrupt routine for irq 5 (XT winchester)
 	hwint_master	5

align	16
hwint06:		; Interrupt routine for irq 6 (floppy)，软盘中断
 	hwint_master	6

align	16
hwint07:		; Interrupt routine for irq 7 (printer)，打印机中断
 	hwint_master	7
;----------------------------------------------------------------------------
%macro  hwint_slave 1
    ; 1 在调用对于中断的处理例程前，先屏蔽当前中断，防止短时间内连续发生好几次同样的中断
    in al, INT_M_CTLMASK    ; 取出 主8259A 当前的屏蔽位图
    or al, (1 << (%1 - 8)) ; 将该中断的屏蔽位置位，表示屏蔽它
    out INT_M_CTLMASK, al   ; 输出新的屏蔽位图，屏蔽该中断

    ; 2 重新启用 主从8259A 和中断响应；因为 从8259A 的中断会级联导致 主8259A也被关闭，所以需要两个都重新启用
    mov al, EOI
    out INT_M_CTL, al       ; 设置 EOI 位，重新启用 主8259A
    nop
    out INT_S_CTL, al       ; 设置 EOI 位，重新启用 从8259A
    sti                     ; 重新启动中断响应

    ; 3 现在调用中断处理例程
    push %1                 ; 压入中断向量号作为参数
    call [irq_handler_table + (4 * %1)] ; 调用中断处理程序表中的相应处理例程，返回值存放在 eax 中
    add esp, 4              ; 清理堆栈

    ; 4 最后，判断用户的返回值，如果是DISABLE(0)，我们就直接结束；如果不为0，那么我们就重新启用当前中断
    cli                     ; 先将中断响应关闭，这个时候不允许其它中断的干扰
    cmp eax, DISABLE
    je .0                   ; 返回值 == DISABLE，直接结束中断处理
    ; 返回值 != DISABLE，重新启用当前中断
    in al, INT_M_CTLMASK    ; 取出 主8259A 当前的屏蔽位图
    and al, ~(1 <<(%1 - 8))      ; 将该中断的屏蔽位复位，表示启用它
    out INT_M_CTLMASK, al   ; 输出新的屏蔽位图，启用该中断
    ; 这里不需要使用 sti 指令重新启用中断响应，因为在输出新的屏蔽位图到 8259A 后，中断会重新开始响应
.0:
    ret
%endmacro
;----------------------------------------------------------------------------
align	16
hwint08:		; Interrupt routine for irq 8 (realtime clock).
 	hwint_slave	8

align	16
hwint09:		; Interrupt routine for irq 9 (irq 2 redirected)
 	hwint_slave	9

align	16
hwint10:		; Interrupt routine for irq 10
 	hwint_slave	10

align	16
hwint11:		; Interrupt routine for irq 11
 	hwint_slave	11

align	16
hwint12:		; Interrupt routine for irq 12
 	hwint_slave	12

align	16
hwint13:		; Interrupt routine for irq 13 (FPU exception)
 	hwint_slave	13

align	16
hwint14:		; Interrupt routine for irq 14 (AT winchester)
 	hwint_slave	14

align	16
hwint15:		; Interrupt routine for irq 15
 	hwint_slave	15
;============================================================================
;   异常处理
;----------------------------------------------------------------------------
divide_error:
	push	0xffffffff	; 没有错误代码，用0xffffffff表示
	push	0		    ; 中断向量号	= 0
	jmp	exception
single_step_exception:
	push	0xffffffff	; 没有错误代码，用0xffffffff表示
	push	1		    ; 中断向量号	= 1
	jmp	exception
nmi:
	push	0xffffffff	; 没有错误代码，用0xffffffff表示
	push	2		    ; 中断向量号	= 2
	jmp	exception
breakpoint_exception:
	push	0xffffffff	; 没有错误代码，用0xffffffff表示
	push	3		    ; 中断向量号	= 3
	jmp	exception
overflow:
	push	0xffffffff	; 没有错误代码，用0xffffffff表示
	push	4		    ; 中断向量号	= 4
	jmp	exception
bounds_check:
	push	0xffffffff	; 没有错误代码，用0xffffffff表示
	push	5		    ; 中断向量号	= 5
	jmp	exception
inval_opcode:
	push	0xffffffff	; 没有错误代码，用0xffffffff表示
	push	6		    ; 中断向量号	= 6
	jmp	exception
copr_not_available:
	push	0xffffffff	; 没有错误代码，用0xffffffff表示
	push	7		    ; 中断向量号	= 7
	jmp	exception
double_fault:
	push	8		    ; 中断向量号	= 8
	jmp	exception
copr_seg_overrun:
	push	0xffffffff	; 没有错误代码，用0xffffffff表示
	push	9		    ; 中断向量号	= 9
	jmp	exception
inval_tss:
	push	10		    ; 中断向量号	= 10
	jmp	exception
segment_not_present:
	push	11		    ; 中断向量号	= 11
	jmp	exception
stack_exception:
	push	12		    ; 中断向量号	= 12
	jmp	exception
general_protection:
	push	13		    ; 中断向量号	= 13
	jmp	exception
page_fault:
	push	14		    ; 中断向量号	= 14
	jmp	exception
copr_error:
	push	0xffffffff	; 没有错误代码，用0xffffffff表示
	push	16		    ; 中断向量号	= 16
	jmp	exception

exception:
	call	exception_handler
	add	esp, 4 * 2	    ; 让栈顶指向 EIP，堆栈中从顶向下依次是：EIP、CS、EFLAGS
    ret                 ; 系统已将异常解决，继续运行！

;============================================================================
;   系统进入宕机
;----------------------------------------------------------------------------
down_run:
    hlt
    jmp down_run

