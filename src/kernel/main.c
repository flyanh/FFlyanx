/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/2/20.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 *  该文件包含包含了内核的入口以及一个内核慌乱宕机(panic)函数
 *
 * 该文件的入口点是：
 *  - flyanx_main:      flyanx的主程序入口
 */

#include "kernel.h"
#include "protect.h"
#include <flyanx/common.h>
#include "process.h"

/*=========================================================================*
 *				flyanx_main				   *
 *			    内核主函数
 *=========================================================================*/
void flyanx_main(void){

    printf("#{flyanx_main}-->Hello OS!!!\n");

    /* 启动时钟驱动任务 */
    clock_task();

    /* 进程表的所有表项都被标志为空闲;
     * 对用于加快进程表访问的 p_proc_addr 数组进行初始化。
     */
    register Process_t *proc;
    register int logic_nr;
    for(proc = BEG_PROC_ADDR, logic_nr = -NR_TASKS; proc < END_PROC_ADDR; proc++, logic_nr++) {
        if(logic_nr > 0)    /* 系统服务和用户进程 */
            strcpy(proc->name, "unused");
        proc->logic_nr = logic_nr;
        p_proc_addr[logic_nr_2_index(logic_nr)] = proc;
    }

    /* 初始化多任务支持
     * 为系统任务和系统服务设置进程表，它们的堆栈被初始化为数据空间中的数组
     */
    SysProc_t *sys_proc;
    reg_t sys_proc_stack_base = (reg_t) sys_proc_stack;
    u8_t  privilege;        /* CPU 权限 */
    u8_t rpl;               /* 段访问权限 */
    for(logic_nr = -NR_TASKS; logic_nr <= LOW_USER; logic_nr++) {   /* 遍历整个系统进程 */
        proc = proc_addr(logic_nr);                                 /* 拿到系统进程对应应该放在的进程指针 */
        sys_proc = &sys_proc_table[logic_nr_2_index(logic_nr)];     /* 系统进程项 */
        strcpy(proc->name, sys_proc->name);                         /* 拷贝名称 */
        /* 判断是否是系统任务还是系统服务 */
        if(logic_nr < 0) {  /* 系统任务 */
            if(sys_proc->stack_size > 0){
                /* 如果任务存在堆t栈空间，设置任务的堆栈保护字 */
                proc->stack_guard_word = (reg_t*) sys_proc_stack_base;
                *proc->stack_guard_word = SYS_TASK_STACK_GUARD;
            }
            /* 设置权限 */
            proc->priority = PROC_PRI_TASK;
            rpl = privilege = TASK_PRIVILEGE;
        } else {            /* 系统服务 */
            if(sys_proc->stack_size > 0){
                /* 如果任务存在堆栈空间，设置任务的堆栈保护字 */
                proc->stack_guard_word = (reg_t*) sys_proc_stack_base;
                *proc->stack_guard_word = SYS_SERVER_STACK_GUARD;
            }
            proc->priority = PROC_PRI_SERVER;
            rpl = privilege = SERVER_PRIVILEGE;
        }
        /* 堆栈基地址 + 分配的栈大小 = 栈顶 */
        sys_proc_stack_base += sys_proc->stack_size;

        /* ================= 初始化系统进程的 LDT ================= */
        proc->ldt[CS_LDT_INDEX] = gdt[TEXT_INDEX];  /* 和内核公用段 */
        proc->ldt[DS_LDT_INDEX] = gdt[DATA_INDEX];
        /* ================= 改变DPL描述符特权级 ================= */
        proc->ldt[CS_LDT_INDEX].access = (DA_CR | (privilege << 5));
        proc->ldt[DS_LDT_INDEX].access = (DA_DRW | (privilege << 5));
        /* 设置任务和服务的内存映射 */
        proc->map.base = KERNEL_TEXT_SEG_BASE;
        proc->map.size = 0;     /* 内核的空间是整个内存，所以设置它没什么意义，为 0 即可 */
        /* ================= 初始化系统进程的栈帧以及上下文环境 ================= */
        proc->regs.cs = ((CS_LDT_INDEX << 3) | SA_TIL | rpl);
        proc->regs.ds = ((DS_LDT_INDEX << 3) | SA_TIL | rpl);
        proc->regs.es = proc->regs.fs = proc->regs.ss = proc->regs.ds;  /* C 语言不加以区分这几个段寄存器 */
        proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK | rpl);       /* gs 指向显存 */
        proc->regs.eip = (reg_t) sys_proc->initial_eip;                 /* eip 指向要执行的代码首地址 */
        proc->regs.esp = sys_proc_stack_base;                           /* 设置栈顶 */
        proc->regs.eflags = is_task_proc(proc) ? INIT_TASK_PSW : INIT_PSW;

        /* 进程刚刚初始化，让它处于可运行状态，所以标志位上没有1 */
        proc->flags = CLEAN_MAP;
        /* 如果该进程不是待机任务 或 虚拟硬件，就绪它 */
        if(!is_idle_hardware(logic_nr)) ready(proc);
    }

    /* 设置消费进程，它需要一个初值。因为系统闲置刚刚启动，所以此时闲置进程是一个最合适的选择。
     * 随后在调用下一个函数 lock_hunter 进行第一次进程狩猎时可能会选择其他进程。
     */
    bill_proc = proc_addr(IDLE_TASK);
    proc_addr(IDLE_TASK)->priority = PROC_PRI_IDLE;
    lock_hunter();      /* 让我们看看，有什么进程那么幸运的被抓出来第一个执行 */

    /* 最后,main 的工作至此结束。它的工作到初始化结束为止。restart 的调用将启动第一个任务，
     * 控制权从此不再返回到main。
     *
     * restart 作用是引发一个上下文切换,这样 curr_proc 所指向的进程将运行。
     * 当 restart 执行了第一次时,我们可以说 Flyanx 正在运行-它在执行一个进程。
     * restart 被反复地执行,每当系统任务、服务器进程或用户进程放弃运行机会挂
     * 起时都要执行 restart,无论挂起原因是等待输入还是在轮到其他进程运行时将控制权转交给它们。
     */
    restart();
}

/*=========================================================================*
 *				panic				   *
 *	内核遇到了不可恢复的异常或错误，立即准备宕机
 *=========================================================================*/
PUBLIC void panic(
        _CONST char* msg,        /* 错误消息 */
        int error_no            /* 错误代码 */
){
    /* 当flyanx发现无法继续运行下去的故障时将调用它。典型的如无法读取一个很关键的数据块、
     * 检测到内部状态不一致、或系统的一部分使用非法参数调用系统的另一部分等。
     * 这里对printf的调用实际上是调用printk,这样当正常的进程间通信无法使用时核心仍能够
     * 在控制台上输出信息。
     */

    /* 有错误消息的话，请先打印 */
    if(msg != NIL_PTR){
        printf("\n!***** Flyanx kernel panic: %s *****!\n", msg);
        if(error_no != NO_NUM)
            printf("!*****     error no: 0x%x     *****!", error_no);
        printf("\n");
    }
    /* 好了，可以宕机了 */
    down_run();
}

/*=========================================================================*
 *				idle_task				   *
 *	            待机任务
 *=========================================================================*/
PUBLIC void idle_task(void) {
    /* 本例程是一个空循环，Flyanx 系统没有任何进程就绪时，则会调用本例程。
     * 本例程的循环体使用了 hlt 指令，使其 CPU 暂停工作并处于待机等待状态，
     * 不至于像传统的死循环一样，消耗大量的 CPU 资源。而且在每个待机的过程
     * 中都会保持中断开启，保证待机时间内随时可以响应活动。
     */
    printf("#{IDLE}-> Working...\n");
    while (TRUE)
        level0(halt);
}
