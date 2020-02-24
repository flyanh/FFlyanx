/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2019/11/9.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * 内核所需的所有函数原型
 * 所有那些必须在其定义所在文件外被感知的函数的原型都放在proto.h中。
 *
 * 它使用了_PROTOTYPE技术，这样，Flyanx核心便既可以使用传统的C编译器(由Kernighan和Richie定义)，
 * 例如Minix初始提供的编译器；又可以使用一个现代的ANSI标准C编译器。
 *
 * 这其中的许多函数原型是与系统相关的,包括中断和异常处理例程以及用汇编语言写的一些函数。
 */

#ifndef FLYANX_PROTOTYPE_H
#define FLYANX_PROTOTYPE_H

/* 结构体声明 */



/*================================================================================================*/
/* kernel.asm */
/*================================================================================================*/

/*================================================================================================*/
/* start.c */
/*================================================================================================*/
_PROTOTYPE( void cstart, (void) );

/*================================================================================================*/
/* protect.c */
/*================================================================================================*/
_PROTOTYPE( void init_segment_desc, (SegDescriptor_t *p_desc, phys_bytes base,phys_bytes limit, u16_t attribute) );
_PROTOTYPE( phys_bytes seg2phys, (U16_t seg) );
_PROTOTYPE( void protect_init, (void) );

/*================================================================================================*/
/* kernel_386lib.asm */
/*================================================================================================*/
_PROTOTYPE( void phys_copy, (phys_bytes _src, phys_bytes _dest, phys_bytes _size) );
_PROTOTYPE( void low_print, (char* _str) );


#endif //FLYANX_PROTOTYPE_H
