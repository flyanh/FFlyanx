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

//    printf("caller: %s\n", caller->name);
//    printf("#sys_call->{caller: %d, op: 0x%x, src_dest: %d, msg_ptr: 0x%p}\n",
//           caller->logic_nr , op, src_dest_msgp, msg_ptr);

    /*==============================*/
    /*      处理设置收发件箱        */
    /*==============================*/
    if(op == IN_OUTBOX) {
        msg_vir = (vir_bytes) src_dest_msgp;
        if(msg_vir != 0)
            caller->inbox = (Message_t *) proc_vir2phys(caller, msg_vir);
        if(msg_ptr != NIL_MESSAGE)
            caller->outbox = (Message_t *) proc_vir2phys(caller, msg_ptr);
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

    /* 首先是处理 SEND，它也包含 SEND_REC 里的 SEND 操作 */
    if(op & SEND){

        /* 自己给自己发送消息，会发生死锁！ */
        assert(caller->logic_nr != src_dest_msgp);

        /* 获取调用者消息的物理地址，这一步很重要，因为我们现在处于内核空间，直接对进程虚拟地址操作是没有用的 */
        if(msg_ptr == NIL_MESSAGE)
            msg_phys = (Message_t *) caller->outbox;
        else
            msg_phys = (Message_t *) proc_vir2phys(caller, msg_ptr);

        /* 设置消息源，即对方要知道是谁发来的这条消息，我们需要设置一下 */
        msg_phys->source = caller->logic_nr;

        /* 调用 flyanx_send，完成发送消息的实际处理 */
        rs = flyanx_send(caller, src_dest_msgp, msg_phys);

        /* 如果只是单纯的发送操作，无论成功与否，请直接返回操作代码 */
        if(op == SEND) return rs;

        /* SEND_REC 的话，如果发送失败，也直接返回不再有下文 */
        if(rs != OK) return rs;
    }

    /* 获取调用者消息的物理地址，这一步很重要，因为我们现在处于内核空间，直接对进程虚拟地址操作是没有用的 */
    if(msg_ptr == NIL_MESSAGE)
        msg_phys = (Message_t *) caller->inbox;
    else
        msg_phys = (Message_t *) proc_vir2phys(caller, msg_ptr);

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
        Message_t *msg_phys    /* 调用者物理消息指针 */
){
    /* 发送一条消息从发送进程到接收进程，消息在发送进程的数据空间中，所以我们
     * 需要将其复制到接收进程的数据空间的消息缓冲中。
     */
    register Process_t *target, *next;

    /* 如果用户试图绕过系统服务直接发送消息给系统任务，返回错误，这是禁止的操作 */
    if( is_user_proc(caller) && !is_sys_server(dest) ) return ERROR_BAD_DEST;
    target = proc_addr(dest);

    /* 检查目标进程是否还是一个活动进程 */
    if( is_empty_proc(target) ) return ERROR_BAD_DEST;

    /* 通过‘调用进程’和‘目标进程’相互发送消息来检查是否存在死锁
     * 它确保消息的目标进程没有正在试图向调用进程发送一条反向的
     * 消息，如果出现了，将会发生一个死链，大家互相等待对方的消
     * 息，但是永远等不到结果，形成了死锁。
     */
    if(target->flags & SENDING) {                   /* 对方正在发送一条消息 */
        next = proc_addr(target->send_to);
        while (TRUE) {
            /* 巧了这不，发送链上果真有人也想发送消息给我，为了避免循环死锁，这次消息我主动放弃发送 */
            if(next == caller) return ERROR_LOCKED;
            if(next->flags & SENDING)               /* 只要发送链上的人是处于发消息的状态，继续往下找 */
                next = proc_addr(next->send_to);
            else
                break;  /* 很好，无内鬼，整个发送链上都没有人会再反向发送一条消息给我，说明这次的消息发送很安全 */
        }
    }

    /* 开始最关键的测试
     * 我们首先看一下对方是不是在接收消息的状态上，如果他在等待，我们就问一下他：“你在等谁啊？”
     * 如果他正好在等待我或者是任何人，那么我们就可以给它发送消息，将消息拷贝给它。
     */
    if( (target->flags & (SENDING | RECEIVING)) == RECEIVING    /* RECEIVING | SENDING是为了保证对方不处于 SEND_REC 调用上 */
        && (target->get_form == caller->logic_nr || target->get_form == ANY)) {
        /* 拷贝消息 */
        msg_copy((phys_bytes) msg_phys, (phys_bytes) target->transfer);
        /* 解除对方的堵塞状态 */
        target->flags &= ~RECEIVING;
        if(target->flags == CLEAN_MAP) ready(target);
    } else {
        /* 如果对方并没有堵塞，或者他被堵塞但不是在等待我
         * 那么堵塞我自己（发送消息的人）并开始排队。
         */
        if(caller->flags == CLEAN_MAP) unready(caller);
        caller->flags |= SENDING;
        caller->send_to = dest;
        caller->transfer = msg_phys;

        /* 加入对方的等待队列 */
        next = waiters[dest];       /* 得到对方的等待队列 */
        if(next == NIL_PROC)
            waiters[dest] = caller;
        else {
            while (next->next_waiter != NIL_PROC)
                next = next->next_waiter;
            next->next_waiter = caller;
        }
        caller->next_waiter = NIL_PROC;
    }

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
    /* 一个进程想要接收一条消息
     *
     * 如果对方已经在排队了，那么就接收他的消息并唤醒对方。如果没有所需来源的消息，那么
     * 堵塞自己，告诉外界我正在等待一条消息。在这无需检查参数的有效性，因为用户调用只能
     * 是 send_receive，所以在 flyanx_send 中我们已经检查，而来自系统进程的调用则肯
     * 定是受信任的。
     */
    register Process_t *sender, *prev;

    /* 检查有没有要发消息过来给我并符合我的要求的 */
    if( !(caller->flags & SENDING) ) {          /* 首先，我自己不能处于发送消息的状态中 */
        /* 遍历等待队列，看有木有人给我发消息啊 */
        for(sender = waiters[caller->logic_nr];
            sender != NIL_PROC; prev = sender, sender = sender->next_waiter) {
            /* 当我对发送者无需求 或 对方就是我获取消息的期望，那么可以拿到对方的消息了 */
            if(sender->logic_nr == src || src == ANY) {
                /* 拷贝消息 */
                msg_copy( (phys_bytes) sender->transfer, (phys_bytes) msg_phys );

                /* 现在来处理等待队列的情况
                 * 如果对方是队头：队头改为队列下一个人
                 * 对方处于队头后：对方出队，排在下一个的人顶替他原来的位置
                 */
                if(sender == waiters[caller->logic_nr])
                    waiters[caller->logic_nr] = sender->next_waiter;
                else
                    prev->next_waiter = sender->next_waiter;

                /* 取消对方的发送中状态，这个时候如果对方的堵塞位图已经是干净的，那么可以就绪他 */
                if( (sender->flags &= ~SENDING) == CLEAN_MAP) ready(sender);
                return OK;
            }
        }
    }
    /* 没有找到我期望的对方，堵塞自己 */
    caller->get_form = src;
    caller->transfer = msg_phys;
    if(caller->flags == CLEAN_MAP) unready(caller);
    caller->flags |= RECEIVING;

    return OK;
}
