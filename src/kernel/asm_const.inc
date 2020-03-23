;================================================================================================
; 文件：asmconst.inc
; 作用：内核文件"kernel.asm"所需的常量定义
; 作者： Flyan
; 联系QQ： 1341662010
;================================================================================================
;================================================================================================
; 自定义的一些常量
;================================================================================================

;================================================================================================
; 保护模式相关
;================================================================================================
; Stackframe 栈帧结构中每个属性的偏移
P_STACKBASE     equ 0               ; 栈基地址
GSREG           equ P_STACKBASE     ; GS段寄存器
FSREG           equ GSREG + 4       ; FS段寄存器
ESREG           equ FSREG + 4       ; ES段寄存器
DSREG           equ ESREG + 4       ; DS段寄存器(数据段寄存器)
EDIREG          equ DSREG + 4       ; EDI寄存器
ESIREG          equ EDIREG + 4      ; ESI寄存器
EBPREG          equ ESIREG + 4      ; EBP寄存器
KERNELESPREG    equ EBPREG + 4      ; 内核ESP寄存器
EBXREG          equ KERNELESPREG + 4
EDXREG          equ EBXREG + 4
ECXREG          equ EDXREG + 4
EAXREG          equ ECXREG + 4
RETADDR         equ EAXREG + 4
PCREG           equ RETADDR + 4
CSREG           equ PCREG + 4
PWSREG          equ CSREG + 4
ESPREG          equ PWSREG + 4
SSREG           equ ESPREG + 4
P_STACKTOP      equ SSREG + 4       ; 栈顶
P_LDT_SEL       equ P_STACKTOP      ; LDT选择子
P_LDT           equ P_LDT_SEL + 4   ; LDT
; TSS::SP0 的偏移
TSS3_S_SP0      equ 4

; 硬件中断处理相关 
INT_M_CTL           equ    0x20     ; I/O port for interrupt controller         <Master>
INT_M_CTLMASK       equ    0x21     ; setting bits in this port disables ints   <Master>
INT_S_CTL           equ    0xA0     ; I/O port for second interrupt controller  <Slave>
INT_S_CTLMASK       equ    0xA1     ; setting bits in this port disables ints   <Slave>
; 中断控制器的神奇数字EOI，可以用于控制中断的打开和关闭，当然，这个宏可以被类似功能的引用
EOI                 equ  0x20       ; EOI，发送给8259A端口1，以重新启用中断
DISABLE             equ  0          ; 用于在中断后保持当前中断关闭的代码
ENABLE              equ  EOI	    ; 用于在中断后重新启用当前中断的代码

; 以下选择子值必须与 portect.h 中保持一致！
SELECTOR_TEXT       equ 8                   ; LOADER 里面已经确定了的
SELECTOR_DATA       equ 16                  ; LOADER 里面已经确定了的，可读写数据段
SELECTOR_TSS        equ 32                  ; TSS. 从外层调到内存时 SS 和 ESP 的值从里面获得

SELECTOR_KERNEL_CS  equ SELECTOR_TEXT       ; 内核代码段
SELECTOR_KERNEL_DS  equ SELECTOR_DATA       ; 内核数据段
;================================================================================================
;================================================================================================
