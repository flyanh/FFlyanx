/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/4/14.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * 进程调度相关
 */
#include "kernel.h"
#include <flyanx/common.h>
#include "process.h"

/* 进程正在切换则为TRUE，否则为FALSE；当为TRUE应该禁止硬件中断的产生，不然会
 * 产生一些严重的问题。
 */
PRIVATE bool_t switching = FALSE;

/* 本地函数声明 */
FORWARD _PROTOTYPE( void hunter, (void) );
FORWARD _PROTOTYPE( void schedule, (void) );

/*===========================================================================*
 *				     hunter   				     *
 *				  狩猎一个进程用于下次执行
 *===========================================================================*/
PRIVATE void hunter(void){
    /* 从进程表中抓出一个作为下次运行的进程
     *
     * Flyanx使用和MINIX一样的三个就绪进程队列，分别是系统任务、系统服务、用户进程
     * 我们的调度算法很简单：找到优先级最高的非空队列，并选择队首进程即可。
     * 如果所有队列均为空，则运行闲置进程IDLE。
     *
     * 选择最高优先级队列由本例程完成。
     * 该函数的主要作用是设置curr_proc(系统当前运行的进程)，任何影响到选择
     * 下一个运行进程的对这些队列的改变都要再次调用hunter。无论进程在什么时
     * 候阻塞，都调用curr_proc来重新调度CPU。
     */
    register Process_t* prey;      /* 准备运行的进程 */
    if((prey = ready_head[TASK_QUEUE]) != NIL_PROC) {
        curr_proc = prey;
        printf("%s hunter\n", curr_proc->name);
        return;
    }
    if((prey = ready_head[SERVER_QUEUE]) != NIL_PROC) {
        curr_proc = prey;
        return;
    }
    if((prey = ready_head[USER_QUEUE]) != NIL_PROC) {
        curr_proc = prey;
        return;
    }
    /* 咳咳，本次狩猎失败了，那么只能狩猎 IDLE 待机进程了 */
    prey = proc_addr(IDLE_TASK);
    bill_proc = curr_proc = prey;

    /* 本例程只负责狩猎，狩猎到一个可以执行的进程，而进程执行完毕后的删除或更改在队列中的位置
     * 这种事情我们安排在其他地方去做。
     */
}

/*===========================================================================*
 *				ready					     *
 *				进程就绪
 *===========================================================================*/
PUBLIC void ready(
        register Process_t* proc    /* 要就绪的进程 */
){
    if(is_task_proc(proc)){
        if(ready_head[TASK_QUEUE] != NIL_PROC){
            /* 就绪队列非空，挂到队尾 */
            ready_tail[TASK_QUEUE]->next_ready = proc;
        } else{
            /* 就绪队列是空的，那么这个进程直接就可以运行，并挂在就绪队列头上 */
            curr_proc = ready_head[TASK_QUEUE] = proc;
        }
        // 队尾指针指向新就绪的进程
        ready_tail[TASK_QUEUE] = proc;      /* 队尾指针 --> 新就绪的进程 */
        proc->next_ready = NIL_PROC;        /* 新条目没有后继就绪进程 */
        return;
    }
    if(is_serv_proc(proc)){
        /* 同上 */
        if(ready_head[SERVER_QUEUE] != NIL_PROC){
            ready_tail[SERVER_QUEUE]->next_ready = proc;
        } else{
            curr_proc = ready_head[SERVER_QUEUE] = proc;
        }
        ready_tail[SERVER_QUEUE] = proc;
        proc->next_ready = NIL_PROC;
        return;
    }
    /* 用户进程的处理稍微有所不同
     * 我们将用户进程添加到队列的最前面。（对于受I/O约束的进程来说更公平一些。）
     */
    if(ready_head[USER_QUEUE] != NIL_PROC){
        ready_tail[USER_QUEUE] = proc;
    }
    proc->next_ready = ready_head[USER_QUEUE];
    ready_head[USER_QUEUE] = proc;
}

/*===========================================================================*
 *				unready					     *
 *				进程取消就绪
 *===========================================================================*/
PUBLIC void unready(
        register Process_t* proc    /* 堵塞的进程 */
){
    /* 将一个不再就绪的进程从其队列中删除，即堵塞。
     * 通常它是将队列头部的进程去掉，因为一个进程只有处于运行状态才可被阻塞。
     * unready 在返回之前要一般要调用 hunter。
     */
    register Process_t *xp;
    if(is_task_proc(proc)){        /* 系统任务？ */
        /* 如果系统任务的堆栈已经不完整，内核出错。 */
        if(*proc->stack_guard_word != SYS_TASK_STACK_GUARD){
            panic("stack over run by task", proc->logic_nr);
        }

        xp = ready_head[TASK_QUEUE];   /* 得到就绪队列头的进程 */
        if(xp == NIL_PROC) return;     /* 并无就绪的系统任务 */
        if(xp == proc){
            /* 如果就绪队列头的进程就是我们要让之堵塞的进程，那么我们将它移除出就绪队列 */
            ready_head[TASK_QUEUE] = xp->next_ready;
            if(proc == curr_proc) {
                /* 如果堵塞的进程就是当前正在运行的进程，那么我们需要重新狩猎以得到一个新的运行进程 */
                hunter();
            }
            return;
        }
        /* 如果这个进程不在就绪队列头，那么搜索整个就绪队列寻找它 */
        while (xp->next_ready != proc){
            xp = xp->next_ready;
            if (xp == NIL_PROC) return;   /* 到边界了，说明这个进程根本就没在就绪队列内 */
        }
        /* 找到了，一样，从就绪队列中移除它 */
        xp->next_ready = xp->next_ready->next_ready;
        /* 如果之前队尾就是要堵塞的进程，那么现在我们需要重新调整就绪队尾指针（因为它现在指向了一个未就绪的进程） */
        if (ready_tail[TASK_QUEUE] == proc) ready_tail[TASK_QUEUE] = xp;   /* 现在找到的xp进程是队尾 */
    } else if(is_serv_proc(proc)){     /* 系统服务 */
        /* 所作操作同上的系统任务一样 */
        xp = ready_head[SERVER_QUEUE];
        if(xp == NIL_PROC) return;
        if(xp == proc){
            ready_head[SERVER_QUEUE] = xp->next_ready;
            /* 这里注意，因为不是系统任务，我们不作那么严格的判断了 */
            hunter();
            return;
        }
        while (xp->next_ready != proc){
            xp = xp->next_ready;
            if (xp == NIL_PROC) return;
        }
        xp->next_ready = xp->next_ready->next_ready;
        if (ready_tail[SERVER_QUEUE] == proc) ready_tail[SERVER_QUEUE] = xp;
    } else {                           /* 用户进程 */
        xp = ready_head[USER_QUEUE];
        if(xp == NIL_PROC) return;
        if(xp == proc){
            ready_head[USER_QUEUE] = xp->next_ready;
            /* 这里注意，因为不是系统任务，我们不作那么严格的判断了 */
            hunter();
            return;
        }
        while (xp->next_ready != proc){
            xp = xp->next_ready;
            if (xp == NIL_PROC) return;
        }
        xp->next_ready = xp->next_ready->next_ready;
        if (ready_tail[USER_QUEUE] == proc) ready_tail[USER_QUEUE] = xp;
    }
}

/*===========================================================================*
 *				schedule					     *
 *			     进程调度
 *===========================================================================*/
PRIVATE void schedule(void){
    /* 这个调度程序只针对于用户进程
     * 尽管多数调度决策实在一个进程阻塞或解除阻塞时作出的，但调度仍要考虑
     * 到当前用户进程时间片用完的情况。这种情况下，时钟任务调度 schedule 来将
     * 就绪用户进程队首的进程移到队尾。
     * 该算法的结果是将用户进程按时间片轮转方式运行。文件系统、内存管理器
     * 和I/O任务绝不会被放在队尾，因为它们肯定不会运行得太久。这些进程可以
     * 被认为是非常可靠的，因为它们是我们编写的，而且在完成要做的工作后将堵塞。
     */

    /* 如果没有准备好的用户进程，请返回 */
    if(ready_head[USER_QUEUE] == NIL_PROC) return;

    /* 将队首的进程移到队尾 */
    Process_t* tmp;
    tmp = ready_head[USER_QUEUE]->next_ready;
    ready_tail[USER_QUEUE]->next_ready = ready_head[USER_QUEUE];
    ready_tail[USER_QUEUE] = ready_tail[USER_QUEUE]->next_ready;
    ready_head[USER_QUEUE] = tmp;
    ready_tail[USER_QUEUE]->next_ready = NIL_PROC;  /* 队尾没有后继进程 */
    /* 汉特儿 */
    hunter();
}

/*===========================================================================*
 *				schedule_stop					     *
 *			     停止进程调度
 *===========================================================================*/
PUBLIC void schedule_stop(void){
    /* 本例程只针对用户进程，使其用户进程不能再被调度,通常在系统宕机时
     * 本例程会被调用，因为这时候系统及其不可靠。本例程的实现也非常简单
     * ，让用户就绪队列为空即可，这样调度程序就找不到任何用户进程了。
     */

    ready_head[USER_QUEUE] = NIL_PROC;
}

/*==========================================================================*
 *				    lock_hunter				    *
 *				    加锁的，安全的进程狩猎例程
 *==========================================================================*/
PUBLIC void lock_hunter(void){
    switching = TRUE;
    hunter();
    switching = FALSE;
}

/*==========================================================================*
 *				    lock_ready				    *
 *				    加锁的，安全的进程就绪例程
 *==========================================================================*/
PUBLIC void lock_ready(Process_t* proc){
    switching = TRUE;
    ready(proc);
    switching = FALSE;
}

/*==========================================================================*
 *				lock_unready				    *
 *				加锁的，安全的进程堵塞例程
 *==========================================================================*/
PUBLIC void lock_unready(Process_t* proc){
    switching = TRUE;
    unready(proc);
    switching = FALSE;
}

/*==========================================================================*
 *				lock_schedule				    *
 *				加锁的进程调度方法
 *==========================================================================*/
PUBLIC void lock_schedule(void)
{
    switching = TRUE;
    schedule();
    switching = FALSE;
}

