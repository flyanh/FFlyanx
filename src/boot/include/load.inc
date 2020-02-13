; Copyright (C) 2007 Free Software Foundation, Inc.
; See the copyright notice in the file /usr/LICENSE.
; Created by flyan on 2019/11/9.
; QQ: 1341662010
; QQ-Group:909830414
; gitee: https://gitee.com/flyanh/
;
; 包含了启动内核的一些信息
; 软盘启动/硬盘启动都需要这个文件

;; 内核挂载点
; 注意：1、必须与 MAKEFILE 中参数 -Ttext 的值相等!!
;      2、这是个物理地址而非仅仅是个偏移，如果 -Ttext 的值为 0x328000，则它的值也应该是 0x328000。
KERNEL_ENTRY_POINT_PHY_ADDR     equ 0x1000

; 页目录开始地址:		1M
PAGE_DIR_BASE		equ	0x100000
; 页表开始地址:		1M + 4K
PAGE_TABLE_BASE		equ	0x101000

; LOADER.BIN 被加载的位置　--- 段地址
LOADER_SEG        equ     0x9000
; LOADER.BIN 被加载的位置　--- 偏移地址
LOADER_OFFSET      equ     0x100
; LOADER.BIN 被加载到的位置　--- 物理地址　(= LOADER_SEG * 10h)
LOADER_PHY_ADDR     equ LOADER_SEG * 10h

; KERNEL.BIN 被加载到的位置 ----  段地址
KERNEL_SEG	        equ	    0x7000
; KERNEL.BIN 被加载到的位置 ---- 偏移地址
KERNEL_OFFSET	equ	    0
; KERNEL.BIN 被加载到的位置　--- 物理地址　(= KERNEL_SEG * 10h)
KERNEL_PHY_ADDR	equ	KERNEL_SEG * 10h

;; 为kernel.bin装载保留的内存大小(字节)，内核文件大小不应该超过这个大小！
KERNEL_HAVE_SPACE   equ LOADER_PHY_ADDR - KERNEL_PHY_ADDR

;; 引导（启动）参数
; 注意：
;   以下的宏应该和C代码里的同步，它们在include/flyanx/config.h下
BOOT_PARAM_ADDR     equ 0x700
BOOT_PARAM_MAGIC    equ 0x328
