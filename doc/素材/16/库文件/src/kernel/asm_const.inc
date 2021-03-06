;================================================================================================
; 文件：asmconst.inc
; 作用：内核文件所需的常量定义
; 作者： Flyan
; 联系QQ： 1341662010
;================================================================================================
;================================================================================================
; 自定义的一些常量
;================================================================================================

;================================================================================================
; 保护模式相关
;================================================================================================


; 以下选择子值必须与 portect.h 中保持一致！
SELECTOR_TEXT       equ 8                   ; LOADER 里面已经确定了的
SELECTOR_DATA       equ 16                  ; LOADER 里面已经确定了的，可读写数据段
SELECTOR_TSS        equ 32                  ; TSS. 从外层调到内存时 SS 和 ESP 的值从里面获得

SELECTOR_KERNEL_CS  equ SELECTOR_TEXT       ; 内核代码段
SELECTOR_KERNEL_DS  equ SELECTOR_DATA       ; 内核数据段
;================================================================================================
;================================================================================================
