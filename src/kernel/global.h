/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2019/11/9.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * 内核所需要的全局变量
 */

#ifndef FLYANX_GLOBAL_H
#define FLYANX_GLOBAL_H

/* 当该文件被包含在定义了宏_TABLE的 table.c中时，宏EXTERN的定义被取消。 */
#ifdef _TABLE
#undef EXTERN
#define EXTERN
#endif

/* GDT、IDT、etc.. */
extern SegDescriptor_t gdt[];
EXTERN u8_t gdt_ptr[6];                             /* GDT指针，0~15：Limit 16~47：Base */
EXTERN u8_t idt_ptr[6];                             /* IDT指针，同上 */
EXTERN int display_position;                        /* low_print函数需要它标识显示位置 */

/* 内核内存 */
EXTERN phys_bytes data_base;	/* 内核数据段基地址 */
EXTERN MemoryMap_t kernel_map;  /* 内核内存映像 */

/* 其他 */
EXTERN BootParams_t* boot_params;    /* 引导参数 */


#endif // FLYANX_GLOBAL_H
