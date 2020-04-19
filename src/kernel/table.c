/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2019/11/10.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * 本文件其编译生成的目标文件将包含所有的核心数据结构,我们已经看到许多这类数据结构定义在glo.h 和proc.h中。
 *
 * 文件开始就定义了宏_TABLE,正好位于 #include语句之前。该定义将导致EXTERN被定义为空串,于是为EXTERN之后
 * 的所有数据声明分配存储空间。除了global.h 和 process.h中的结构以外,tty.h中定义的由终端任务使用的几个全局变量
 * 也都在这里分配存储空间。
 *
 * 这个方法非常巧妙，使用这个方法，一次头文件extern声明全局变量，在这个文件中导入，又将真正的变量声明出来并
 * 分配空间。
 */

#define _TABLE

#include "kernel.h"
#include <stdlib.h>
#include <termios.h>
#include <flyanx/common.h>
#include "process.h"

/* === 堆栈相关 === */
/* 一个 512 字节 的小栈 */
#define SMALL_STACK (128 * sizeof(char*))
/* 这是一个普通堆栈大小，1KB */
#define NORMAL_STACK (256 * sizeof(char*))

/* 待机任务堆栈 */
#define IDLE_TASK_STACK     (19 * sizeof(char*))
/* 虚拟硬件栈 */
#define HARDWARE_STACK      0

/* 所有系统进程的栈空间总大小 */
#define TOTAL_SYS_PROC_STACK    ( IDLE_TASK_STACK )

/* 所有系统进程堆栈的堆栈空间。 （声明为（char *）使其对齐。） */
PUBLIC char *sys_proc_stack[TOTAL_SYS_PROC_STACK / sizeof(char *)];

/* === 系统进程表，包含系统任务以及系统服务 === */
PUBLIC SysProc_t sys_proc_table[] = {
        /* ************************* 系统任务 ************************* */
        /* 待机任务 */
        { idle_task, IDLE_TASK_STACK, "IDLE" },
        /* 虚拟硬件任务，只是占个位置 - 用作判断硬件中断 */
        { 0, HARDWARE_STACK, "HARDWARE" },


        /* ************************* 系统服务 ************************* */
};

/* 虽然已经尽量将所有用户可设置的配置消息单独放在 include/flyanx/config.h 中,但是在将系统任务表的大小与
 * NR_TASKS 相匹配时仍可能会出现错误。在这里我们使用了一个小技巧对这个错误进行检测。方法是在这里声明一个
 * dummy_task_table，声明的方式是假如发生了前述的错误,则 dummy_task_table 的大小将是非法的,从而导致编译错误。
 * 由于哑数组声明为 extern ,此处并不会为它分配空间(其他地方也不为其分配空间)。因为在代码中任何地方都不会
 * 引用到它,所以编译器和地址空间不会受任何影响。
 *
 * 简单解释：减去的是 ORIGIN，这些都不属于系统进程。
 */
//#define NKT (sizeof(task_table) / sizeof(Task_t) - (ORIGIN_PROC_NR + 1))
#define NKT ( sizeof(sys_proc_table) / sizeof(SysProc_t) )

extern int dummy_tasktab_check[NR_TASKS + NR_SERVERS == NKT ? 1 : -1];

