/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/3/16.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * 时钟驱动任务
 */
#include "kernel.h"
#include <flyanx/common.h>
#include "process.h"

/* 用户进程使用时间片轮转算法，这里可以对轮转时间进行配置 */
#define SCHEDULE_MILLISECOND    130         /* 用户进程调度的频率（毫秒），根据喜好设置就行 */
#define SCHEDULE_TICKS          (SCHEDULE_MILLISECOND / ONE_TICK_MILLISECOND)  /* 用户进程调度的频率（滴答） */

/* 时钟, 8253 / 8254 PIT (可编程间隔定时器)参数 */
#define TIMER0          0x40	/* 定时器通道0的I/O端口 */
#define TIMER1          0x41	/* 定时器通道1的I/O端口 */
#define TIMER2          0x42	/* 定时器通道2的I/O端口 */
#define TIMER_MODE      0x43	/* 用于定时器模式控制的I/O端口 */
#define RATE_GENERATOR  0x34    /* 00-11-010-0
                                 * Counter0 - LSB the MSB - rate generator - binary
                                 */
#define TIMER_FREQ		    1193182L    /* clock frequency for timer in PC and AT */
#define TIMER_COUNT  (TIMER_FREQ / HZ)  /* initial value for counter*/
#define CLOCK_ACK_BIT	    0x80		/* PS/2 clock interrupt acknowledge bit */

/* 时钟任务的变量 */
PRIVATE Message_t msg;
PRIVATE clock_t ticks;          /* 时钟运行的时间(滴答数)，也是开机后时钟运行的时间 */
PRIVATE time_t realtime;        /* 时钟运行的时间(s)，也是开机后时钟运行的时间 */

/* 由中断处理程序更改的变量 */
PRIVATE clock_t schedule_ticks = SCHEDULE_TICKS;    /* 用户进程调度时间，当为0时候，进行程序调度 */
PRIVATE Process_t *last_proc;                       /* 最后使用时钟任务的用户进程 */

/* 本地函数 */
FORWARD _PROTOTYPE( int clock_handler, (int irq) );
FORWARD _PROTOTYPE( void clock_init, (void) );

/*===========================================================================*
 *				clock_task				     *
 *			    时钟驱动任务
 *===========================================================================*/
PUBLIC void clock_task(void){

    /* 初始化时钟 */
    clock_init();

    /* 初始化收发件箱 */
    io_box(&msg);

    printf("#{CLOCK}-> Working...\n");
    while(TRUE) {
        /* 等待外界消息 */
        rec(ANY);

        /* 提供服务前，校准时间 */
        interrupt_lock();
        realtime = ticks / HZ;  /* 计算按秒算的机器真实时间 */
        interrupt_unlock();

        /* 提供服务 */
        switch (msg.type) {

            default:    panic("#{CLOCK}-> Clock task got bad message request.\n", msg.type);
        }

        /* 根据处理结果，发送回复消息 */
        msg.type = OK;          /* 时钟驱动无可能失败的服务 */
        sen(msg.source);    /* 回复 */
    }
}

/*===========================================================================*
 *				clock_handler				     *
 *				时钟中断处理例程
 *===========================================================================*/
PRIVATE int clock_handler(int irq) {
    register Process_t *target;

    /* 获取当前使用时钟的进程 */
    if(kernel_reenter)  /* 发送中断重入，说明当前处于核心代码段，被中断的进程使用虚拟硬件 */
        target = proc_addr(HARDWARE);
    else                /* 正常中断，被中断的进程就是当前运行的进程 */
        target = curr_proc;

    /* 计时 */
    ticks++;

    /* 记账：给使用了系统资源的用户进程记账 */
    target->user_time++;        /* 用户时间记账 */
    if(target != bill_proc && target != proc_addr(HARDWARE))
        bill_proc->sys_time++;  /* 当前进程不是计费的用户进程，那么它应该是使用了系统调用陷入了内核，记录它的系统时间 */

    /* 重置用户进程调度时间片 */
    if(--schedule_ticks == 0) {
        schedule_ticks = SCHEDULE_TICKS;
        last_proc = bill_proc;  /* 记录最后一个消费进程 */
    }

    return ENABLE;  /* 返回ENABLE，使其再能发生时钟中断 */
}

/*===========================================================================*
 *				clock_init				     *
 *				时钟初始化
 *===========================================================================*/
PRIVATE void clock_init(void) {
    /* 设置 8253定时器芯片 的模式和计数器Counter 0以产生每秒 100 次的
     * 时钟滴答中断，即每 10ms 产生一次中断。
     */

    /* 1 先写入我们使用的模式到 0x43 端口，即模式控制寄存器中 */
    out_byte(TIMER_MODE, RATE_GENERATOR);
    /* 2 写入计数器值的低 8 位再写入高 8 位到 Counter 0 端口 */
    out_byte(TIMER0, (u8_t)TIMER_COUNT);
    out_byte(TIMER0, (u8_t)(TIMER_COUNT >> 8));

    /* 设置时钟中断处理例程并打开其中断响应 */
    put_irq_handler(CLOCK_IRQ, clock_handler);
    enable_irq(CLOCK_IRQ);
}


