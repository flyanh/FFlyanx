/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/2/25.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 *  该文件是内核开始C语言开发后的第一个被执行的C函数，所以它包含一个
 * cstart函数，它完成内核的一些准备工作，为跳入内核主函数做准备。
 *
 * 该文件的入口点是：
 *  - cstart:      进入内核主函数前做一些准备工作
 */


#include "kernel.h"
#include "protect.h"
#include "assert.h"
INIT_ASSERT

int test_int(int irq){
    printf("i am new int handler!\n");
    return DISABLE;
}

/*=========================================================================*
 *				cstart				   *
 *			进入内核主函数前做一些准备工作
 *=========================================================================*/
PUBLIC void cstart(void){

    /* 初始化显示位置 */
    display_position = (80 * 6 + 2 * 0) * 2;
    low_print("#{cstart}-->called\n");

    /* 建立保护机制以及中断表 */
    protect_init();

    /* 初始化硬件中断机制 */
    interrupt_init();

    /* 测试是否能正常注册新中断处理例程 */
    put_irq_handler(3, test_int);
    enable_irq(3);          /* 打开 3 号中断请求 */

    /* 加载引导参数 */
    u32_t* p_boot_params = (u32_t*)BOOT_PARAM_ADDR;
    /* 断言：魔数正常 */
    assert(p_boot_params[BP_MAGIC] == BOOT_PARAM_MAGIC);
    /* 魔数正常，让我们的引导参数指针指向它 */
    boot_params = (BootParams_t*)(BOOT_PARAM_ADDR + 4);

}



