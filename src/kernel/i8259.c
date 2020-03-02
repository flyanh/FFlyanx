/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/3/2.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 */

#include "kernel.h"
#include "assert.h"
INIT_ASSERT

/* 本地函数声明 */
FORWARD _PROTOTYPE( int default_irq_handler, (int irq) );

/*=========================================================================*
 *				interrupt_init				   *
 *			    中断初始化
 *=========================================================================*/
PUBLIC void interrupt_init(void){

    /* 1  向端口20H(主片)或a0H(从片)写入ICW1 */
    /* 00010001b -> 17 */
    out_byte(INT_M_CTL, 17);
    out_byte(INT_S_CTL, 17);
    /* 2 向端口21H(主片)或a1H(从片)写入ICW2 */
    /* 写入我们80386下的中断向量 */
    out_byte(INT_M_CTL, INT_VECTOR_IRQ0);
    out_byte(INT_S_CTL, INT_VECTOR_IRQ8);
    /* 3 向端口21H(主片)或a1H(从片)写入ICW3 */
    /* 主：00000100b -> 4，从：00000010b -> 2 */
    out_byte(INT_M_CTL, 4);
    out_byte(INT_S_CTL, 2);
    /* 4 向端口21H(主片)或a1H(从片)写入ICW4 */
    /* 0000001b -> 1 */
    out_byte(INT_M_CTL, 1);
    out_byte(INT_S_CTL, 1);

    /* 由于现在还没有配置中断例程，我们屏蔽所有中断，使其都不能发生 */
    out_byte(INT_M_CTLMASK, 0xff);
    out_byte(INT_S_CTLMASK, 0xff);

    /* 最后，我们初始化中断处理程序表，给每一个中断设置一个默认的中断处理例程 */
    int i = 0;
    for(; i < NR_IRQ_VECTORS; i++) {
        irq_handler_table[i] = default_irq_handler;
    }

    /* 打开3、9号硬件中断，用于测试中断机制 */
    enable_irq(3);          /* 对应于中断向量号 = INT_VECTOR_IRQ0 + 3 */
    enable_irq(9);          /* 对应于中断向量号 = INT_VECTOR_IRQ0 + 9  */

}

/*=========================================================================*
 *				put_irq_handler				   *
 *			   设置并注册中断处理例程
 *=========================================================================*/
PUBLIC void put_irq_handler(int irq, irq_handler_t handler){
    /* 一旦一个硬件的驱动编写完成，那么就可以调用本例程来
     * 为其设置真正的中断处理例程了，它将会替换掉初始化时
     * 默认的spurious_irq例程。
     */

    /* 断言：中断向量处于正常范围 */
    assert(irq > 0 && irq < NR_IRQ_VECTORS);

    /* 注册过了？那么什么也不做 */
    if(irq_handler_table[irq] == handler) return;

    /* 断言：该中断已初始化过 */
    assert(irq_handler_table[irq] == default_irq_handler);

    /* 开始设置
     * 先关闭对应的中断，再将中断处理例程替换旧的
     */
    disable_irq(irq);
    irq_handler_table[irq] = handler;

    /* 好了，就这些了 */
}

/*=========================================================================*
 *				default_irq_handler				   *
 *			    默认的中断处理例程
 *=========================================================================*/
PRIVATE int default_irq_handler(int irq){
    printf("I am a interrupt, my name is int %d\n", irq);
    return DISABLE; /* 响应完毕不需要重新打开中断 */
}



