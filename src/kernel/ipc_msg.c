/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/4/19.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * IPC（Inter-Process Communication，进程间通信）
 * Flyanx提供了消息会合通信机制，即不管是发送还是接收消息，如果对方
 * 不能及时给出响应，那么进程都会进入到堵塞状态，直至等到对方有合适
 * 的机会完成会合才会解除堵塞。
 *
 * 本文件是实现微内核的核心，应当仔细研究。同时，微内核一直以来被诟病的性能问题。
 * 大部分性能消耗都来自于消息通信，因为发送消息需要在内存中拷贝消息，而这项
 * 工作需要消耗很多的系统资源，所以如果想优化 flyanx 内核，第一步就是解决消息
 * 的资源消耗问题。
 */
#include "kernel.h"
#include <flyanx/common.h>
#include "process.h"
#include "assert.h"
INIT_ASSERT

/* 等待队列
 *
 * 简单来说，这个等待队列，是想发消息给我的其他人排的一个队列。举个例子，我是女神，每天给我送花可以提高
 * 好感度，但是我一次只能收一朵花，所以追我的人，必须得排成一个队伍，我就可以一个一个收花了。
 */
PRIVATE Process_t *waiters[NR_TASKS + NR_SERVERS + NR_PROCS];   /* 每个进程有一个自己的等待队列，所以这是一个数组 */

/*===========================================================================*
 *				sys_call				     *
 *				系统调用
 *===========================================================================*/
PUBLIC int sys_call(
        int op,             /* 执行的操作：发送，接收，或发送并等待对方响应，以及设置收发件箱 */
        int src_dest_msgp,  /* 消息来源 / 消息目的地 / 消息地址 */
        Message_t *msg_ptr  /* 消息指针 */
){
    register Process_t *caller;
    Message_t *msg_phys;
    vir_bytes msg_vir;
    int rs;

    caller = curr_proc;     /* 获取调用者 */

    printf("caller: %s\n", caller->name);
    printf("#sys_call->{caller: %d, op: 0x%x, src_dest: %d, msg_ptr: 0x%p}\n",
           caller->logic_nr , op, src_dest_msgp, msg_ptr);

    /*==============================*/
    /*      处理设置收发件箱        */
    /*==============================*/
    if(op == IN_OUTBOX) {
        msg_vir = (vir_bytes) src_dest_msgp;
        if(msg_vir != 0)
            caller->inbox = (Message_t*) msg_vir;
        if(msg_ptr != NIL_MESSAGE)
            caller->outbox = msg_ptr;
        return OK;
    }

    /*==============================*/
    /*      消息通信前做一些检查     */
    /*==============================*/

    /* 检查并保证该消息指定的源进程目标地址合法，不合法直接返回错误代码 ERROR_BAD_SRC，即错误的源地址 */
    if(!is_ok_src_dest(src_dest_msgp)) return ERROR_BAD_SRC;

    /* 我们继续检查，检查该调用是否从一个用户进程发出
     * 如果是用户进程，但它请求了一个非 SEND_REC 的请求，那么返回一个 E_NO_PERM 的错误码，表示用户
     * 不能越过服务发送消息给任务。因为 SEND 和 RECEIVE 是给系统进程间消息通信设置的。
     * 用户只能请求 SEND_REC，首先申请发出一条消息，随后接收一个应答，对于用户进程而言，这是唯一
     * 一种被系统允许的系统调用方式。
     */
    if(is_user_proc(caller) && op != SEND_REC) return ERROR_NO_PERM;

    /*==============================*/
    /* 下面开始真正处理消息通信调用机制 */
    /*==============================*/

    /* 获取调用者消息的物理地址，这一步很重要，因为我们现在处于内核空间，直接对进程虚拟地址操作是没有用的 */
    if(msg_ptr == NIL_MESSAGE)
        msg_phys = (Message_t *) proc_vir2phys(caller, caller->outbox);
    else
        msg_phys = (Message_t *) proc_vir2phys(caller, msg_ptr);


    /* 首先是处理 SEND，它也包含 SEND_REC 里的 SEND 操作 */
    if(op & SEND){

        /* 自己给自己发送消息，会发生死锁！ */
        assert(caller->logic_nr != src_dest_msgp);

        /* 设置消息源，即对方要知道是谁发来的这条消息，我们需要设置一下 */
        msg_phys->source = caller->logic_nr;

        /* 调用 flyanx_send，完成发送消息的实际处理 */
        rs = flyanx_send(caller, src_dest_msgp, msg_phys);

        /* 如果只是单纯的发送操作，无论成功与否，请直接返回操作代码 */
        if(op == SEND){
            return rs;
        }

        /* SEND_REC 的话，如果发送失败，也直接返回不再有下文 */
        if(rs != OK){
            return rs;
        }

    }

    /* 处理接收消息操作，同样的，也包括SEND_REC里的REC操作
     * 直接调用接收消息的函数，并返回操作代码，例程结束
     */
    return flyanx_receive(caller, src_dest_msgp, msg_phys);
}

/*===========================================================================*
 *				flyanx_send				     *
 *				处理发送消息调用
 *===========================================================================*/
PUBLIC int flyanx_send(
        Process_t *caller,      /* 调用者，即谁想发送一条消息？ */
        int dest,               /* 目标，即准备发消息给谁？ */
        Message_t *msg_phys     /* 调用者物理消息指针 */
){
    printf("flyanx_send...\n");
    return OK;
}

/*===========================================================================*
 *				flyanx_receive				     *
 *				处理接收消息调用
 *===========================================================================*/
PUBLIC int flyanx_receive(
        Process_t *caller,      /* 调用者，即谁想接收一条消息？ */
        int src,                /* 目标，即准备从谁哪里获取一条消息？ */
        Message_t *msg_phys     /* 调用者物理消息指针 */
){
    printf("flyanx_receive...\n");
    return OK;
}
