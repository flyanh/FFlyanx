/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/2/20.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * 该文件包含flyanx的主程序。
 *
 * 该文件的入口点是：
 *  - flyanx_main:      flyanx的主程序
 */

#include "kernel.h"
#include "assert.h"
INIT_ASSERT             // 使用断言之前需要先初始化

void flyanx_main(void){

    low_print("#{flyanx_main}-->Hello OS!!!\n");

    /* 测试零除异常 */
//    int a = 0;
//    int b = 5 / a;

    /* 测试断言 */
    assert(1 == 3);

    /* 比较测试 */
//    compare(1, >, 3);

    /* 测试内核打印函数 */
    k_printf("#{flyanx_main}-->test msg%d %x\n", 7, 0x328);

    while (1){}
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
        if(error_no != NO_NUM){
            printf("!*****     error no: %d     *****!", error_no);
        }
        printf("\n");
    }
    /* 好了，可以宕机了 */
    down_run();
}

