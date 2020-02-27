/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/2/27.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 */

#include "kernel.h"

/* 异常信息表 */
PRIVATE char* exception_table[] = {
        "#DE Divide Error",                                 /* 除法错误 */
        "#DB RESERVED",                                     /* 调试异常 */
        "—   NMI Interrupt",                                /* 非屏蔽中断 */
        "#BP Breakpoint",                                   /* 调试断点 */
        "#OF Overflow",                                     /* 溢出 */
        "#BR BOUND Range Exceeded",                         /* 越界 */
        "#UD Invalid Opcode (Undefined Opcode)",            /* 无效(未定义的)操作码 */
        "#NM Device Not Available (No Math Coprocessor)",   /* 设备不可用(无数学协处理器) */
        "#DF Double Fault",                                 /* 双重错误 */
        "    Coprocessor Segment Overrun (reserved)",       /* 协处理器段越界(保留) */
        "#TS Invalid TSS",                                  /* 无效TSS */
        "#NP Segment Not Present",                          /* 段不存在 */
        "#SS Stack-Segment Fault",                          /* 堆栈段错误 */
        "#GP General Protection",                           /* 常规保护错误 */
        "#PF Page Fault",                                   /* 页错误 */
        "—   (Intel reserved. Do not use.)",                /* Intel保留，不使用 */
        "#MF x87 FPU Floating-Point Error (Math Fault)",    /* x87FPU浮点数(数学错误) */
        "#AC Alignment Check",                              /* 对齐校验 */
        "#MC Machine Check",                                /* 机器检查 */
        "#XF SIMD Floating-Point Exception",                /* SIMD浮点异常 */
};

/*=========================================================================*
 *				exception_handler				   *
 *			    异常处理例程
 *=========================================================================*/
PUBLIC void exception_handler(
        int int_vector,         /* 异常中断向量 */
        int error_no            /* 异常错误代码 */
){

    /* 非屏蔽中断，我们不予理睬 */
    if(int_vector == 2){
        low_print("!********** Got spurious NMI! **********!\n");
        return;
    }

    /* 简单点，内核发生异常，我们宕机 */
    if(exception_table[int_vector] == NIL_PTR){
        low_print("\n!********** Exception no exist **********!\n");
    } else {
        low_print("\n");
        low_print(exception_table[int_vector]);
        low_print("\n");
    }
    /* 有错误代码？打印它！ */
    if(error_no != 0xffffffff){
        low_print("!********** have error no **********!\n");
    }
    low_print("\n!********** Flyanx has down run, please reboot **********!\n");

}


