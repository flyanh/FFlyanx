/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2019/11/9.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * 包含了在操作系统内部调用以访问操作系统其他服务的C库函数原型。
 * 操作系统向外提供的系统调用，也是通过调用这些库函数去实现的。
 */

#ifndef _FLYANX_SYSLIB_H
#define _FLYANX_SYSLIB_H

#ifndef FLYANX_TYPES_H
#include <sys/types.h>
#endif


/* Flyanx 用户和系统双用库 */
_PROTOTYPE( int send_rec, (int src, Message_t *io_msg) );
_PROTOTYPE( int in_outbox, (Message_t *in_msg, Message_t *out_msg) );

/* Flyanx系统库 */
_PROTOTYPE( int send, (int dest, Message_t* out_msg) );
_PROTOTYPE( int receive, (int src, Message_t* in_msg) );


#endif //_FLYANX_SYSLIB_H
