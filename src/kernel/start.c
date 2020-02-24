/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/2/24.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * 本文件包含启动内核主函数前的准备工作的函数
 *
 * 本文件的入口点是：
 *  - cstart            进入内核主函数前做一些准备工作
 */

#include "kernel.h"
#include "protect.h"

/*=========================================================================*
 *				cstart				   *
 *	    进入内核主函数前做一些准备工作
 *=========================================================================*/
PUBLIC void cstart(void){

    /* 初始化显示位置 */
    display_position = (80 * 6 + 2 * 0) * 2;

    /* 首先，我们以及内核数据段的位置 */
    data_base = seg2phys(SELECTOR_KERNEL_DS);

    /* 建立保护机制以及中断表 */
    protect_init();

    /* 加载引导参数 */
    u32_t *p_boot_params = (u32_t*)BOOT_PARAM_ADDR;
    /* 根据魔数判断引导参数是否正常？ */
    if(p_boot_params[BP_MAGIC] != BOOT_PARAM_MAGIC){
        /* 打印提示信息并死机 */
        low_print("bad boot params, please reboot...");
        for(;;);
    }
    /* 魔数正常，引导参数指针指向该区域 */
    boot_params = (BootParams_t*)(p_boot_params + 4);

}

