/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/3/1.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 *  包含了一些C实用程序，是内核公共的例程，这里面的例程都
 * 不太好分类，所以被放在了这里。
 *
 * 该文件的入口点是：
 *  - k_printf:         内核级打印函数，通过low_print来实现
 *  - bad_assertion:    错误的断言处理
 *  - bad_compare:      错误的断定比较处理
 */

#include "kernel.h"
#include <stdarg.h>
#include <stdio.h>

/*=========================================================================*
 *				k_printf				   *
 *			   内核级格式打印
 *=========================================================================*/
PUBLIC int k_printf(
        const char *fmt,    /* 带'%'格式的字符串 */
        ...                 /* 可变参数 */
){
    va_list ap;
    int len;
    static char buf[160];   /* 这里注意：必须是静态的不然这个函数每次调用都来个
                             * 缓冲区，内核的内存根本不够它造，很快就溢出了！
                             * 内核的打印不会超过两行，所以 160 就够了。
                             */

    /* 准备开始访问可变参数 */
    va_start(ap, fmt);

    /* 调用vsprintf格式化字符串 */
    len = vsprintf(buf, fmt, ap);
    /* 调用low_print函数打印格式化后的字符串 */
    low_print(buf);

    /* 可变参数访问结束 */
    va_end(ap);
    return len;
}


/*=========================================================================*
 *				bad_assertion				   *
 *			    错误的断言处理
 *=========================================================================*/
PUBLIC void bad_assertion(
        char *file,         /* 断言失败代码所在文件 */
        int line,           /* 断言失败代码所在文件的哪一行？ */
        char *what          /* 断言源代码 */
){
    /* 本例程只有在宏DEBUG被定义为TRUE时才对其进行编译，支持assert.h中的宏。 */
    printf("\n\n*==============================================================================*");
    printf("* panic at file://%s(%d): assertion \"%s\" failed\n",
           file, line, what);
    printf("*==============================================================================*\n");
    panic("bad assertion", NO_NUM);
}

/*=========================================================================*
 *				bad_compare				   *
 *			  错误的断定比较处理
 *=========================================================================*/
PUBLIC void bad_compare(
        char *file,         /* 断定比较失败所在文件 */
        int line,           /* 断定比较失败代码所在文件的哪一行？ */
        int lhs,            /* 左边的比较数 */
        char *what,         /* 断定比较源代码 */
        int rhs             /* 右边的比较数 */
){
    /* 本例程只有在宏DEBUG被定义为TRUE时才对其进行编译，支持assert.h中的宏。 */
    printf("\n\n*==============================================================================*");
    printf("* panic at file://%s(%d): compare \"%s\" failed\n",
           file, line, what);
    printf("*==============================================================================*\n");
    panic("bad compare", NO_NUM);
}


