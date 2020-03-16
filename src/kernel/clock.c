/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/3/16.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 */
#include "kernel.h"

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

/*  */
PRIVATE clock_t ticks;                  /* 对中断次数计数 */

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

    /* 打开中中断看看效果 */
    interrupt_unlock();

}

/*===========================================================================*
 *				clock_handler				     *
 *				时钟中断处理例程
 *===========================================================================*/
PRIVATE int clock_handler(int irq) {

    ticks++;
    printf(">");

    return ENABLE;
}

/*===========================================================================*
 *				clock_init				     *
 *				时钟初始化
 *===========================================================================*/
PRIVATE void clock_init(void) {
    /* 设置 8253定时器芯片 的模式和计数器Counter 0以产生每秒 100 次的
     * 时钟滴答中断，即每 10ms 产生一次中断。
     */
    printf("#{clock_init}->called\n");

    /* 1 先写入我们使用的模式到 0x43 端口，即模式控制寄存器中 */
    out_byte(TIMER_MODE, RATE_GENERATOR);
    /* 2 写入计数器值的低 8 位再写入高 8 位到 Counter 0 端口 */
    out_byte(TIMER0, (u8_t)TIMER_COUNT);
    out_byte(TIMER0, (u8_t)(TIMER_COUNT >> 8));

    /* 设置时钟中断处理例程并打开其中断响应 */
    put_irq_handler(CLOCK_IRQ, clock_handler);
    enable_irq(CLOCK_IRQ);
}


