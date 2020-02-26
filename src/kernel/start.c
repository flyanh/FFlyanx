/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/2/25.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 */


#include "kernel.h"
#include "protect.h"

/*=========================================================================*
 *				cstart				   *
 *			进入内核主函数前做一些准备工作
 *=========================================================================*/
PUBLIC void cstart(void){

    /* 初始化显示位置 */
    display_position = (80 * 6 + 2 * 0) * 2;
    low_print("-----------> cstart <--------------\n");

    /* 建立保护机制以及中断表 */
    protect_init();

    /* 加载引导参数 */
    u32_t* p_boot_params = (u32_t*)BOOT_PARAM_ADDR;
    if(p_boot_params[BP_MAGIC] != BOOT_PARAM_MAGIC){
        /* 魔数不对，引导参数有问题 */
        low_print("bad boot params, please reboot...\n");
        for(;;);
    }
    /* 魔数正常，让我们的引导参数指针指向它 */
    boot_params = (BootParams_t*)(BOOT_PARAM_ADDR + 4);

}

