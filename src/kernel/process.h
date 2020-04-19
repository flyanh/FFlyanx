/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/3/4.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * Flyanx的进程相关头文件
 */

#ifndef _FLYANX_PROCESS_H
#define _FLYANX_PROCESS_H

/* 进程，一个进程是包含 进程的寄存器信息(栈帧) 和 LDT(本地描述符表) 以及 自定义的一些属性
 * CPU 通过 PCB来调度进程
 */
typedef struct process_s {
    /* ===================================================================================== */
    /* 这里存放 进程的寄存器信息(栈帧) 和 LDT(本地描述符表) 信息，它们由 CPU 使用并调度，和硬件相关 */
    Stackframe_t regs;          /* 进程的栈帧，包含进程自己所有寄存器的信息 */
    reg_t ldt_sel;              /* 进程的 LDT 选择子 */
    SegDescriptor_t ldt[2];     /* 进程的 LDT 的数据，长度2是 LDT_SIZE，它定义在文件 protect.h 中，
                                 * 直接给出就不用导入该头文件了 */
    /* ===================================================================================== */
    /* 从这后面都是用户自定义的属性，和硬件无关 */

    /* 堆栈保护字
     * 用于识别堆栈是否正常，如果被改变那么堆栈已经出现问题
     */
    reg_t* stack_guard_word;

    /* 进程的内存映像
     * 现在包括正文段和数据段（堆栈段），但只有一个而非一个数组，
     * 因为 Flyanx 当前版本并不加以细分这几个段，它们都使用同一个
     * 段区域，所以一个就可以描述了。
     */
    MemoryMap_t map;

    /* 调度相关 */
    /* flags 状态标志位图域中的标志位标识进程的状态。如果其中任一位被置位,则进程将被堵塞
     * 无法运行。各种标志被定义和描述请往下看，如果该进程表项未被使用,则P_SLOT_FREE被置位。
     */
    int flags;
    pid_t pid;                      /* 进程号，用户可见的 */
    u8_t priority;                  /* 权限：任务0/服务1/用户进程3 */
    struct process_s* next_ready;   /* 指向下一个就绪的进程，形成一个队列 */
    int logic_nr;                   /* 进程在进程表中的逻辑编号，主要用于表中的进程快速访问 */
    bool_t int_blocked;             /* 被置位，当目标进程有一条中断消息被繁忙的任务堵塞了 */
    bool_t int_held;                /* 被置位，当目标进程有一条中断消息被繁忙的系统调用挂起保留了 */
    struct process_s* next_held;    /* 被挂起保留的中断过程队列 */

    /* 时间相关 */
    clock_t user_time;              /* 用户时间(以时钟滴答为单位)，即进程自己使用的时间 */
    clock_t sys_time;               /* 系统时间(以时钟滴答为单位)，即进程调用了系统任务的时间，或者说进程本身就是系统任务 */
    clock_t child_user_time;        /* 子进程累积使用的用户时间 */
    clock_t child_sys_time;         /* 子进程累积使用的系统时间 */
    clock_t alarm;                  /* 进程下一次闹钟响起的时间 */

    /* 消息通信相关 */
    Message_t* inbox;               /* 收件箱，当有人发送消息过来将被邮局将消息放在这里，它是一个虚拟地址 */
    Message_t* outbox;              /* 发件箱，当一个进程发送消息给另一个进程，邮局将会从这里获取要发送的消息，
                                     * 同上它也是一个虚拟地址 */
    int get_form;                   /* 当一个进程执行接收操作，但没有发现有任何人想发消息过来时将会堵塞，然后将自己期望
                                     * 接收消息的进程逻辑编号保存在这 */
    int send_to;                    /* 同上，保存要发送消息给谁？ */
    struct process_s* next_waiter;  /* 指向下一个要发送消息给我的人，为了实现等待队列 */

    char name[32];                  /* 这个没啥好说的，就是进程的名称，记得起个好名字哦 */
} Process_t;

/* 系统堆栈的保护字 */
#define SYS_TASK_STACK_GUARD	((reg_t) (sizeof(reg_t) == 2 ? 0xBEEF : 0xDEADBEEF))    /* 任务的 */
#define SYS_SERVER_STACK_GUARD	((reg_t) (sizeof(reg_t) == 2 ? 0xBFEF : 0xDEADCEEF))    /* 服务的 */

/* flags 状态标志位图域中的标志位状态定义
 *
 * 现在一共有三种状态，只要任一状态位被置位，那么进程就会被堵塞
 */
#define CLEAN_MAP       0       /* 干净的状态，进程正在快乐的执行 */
#define NO_MAP		    0x01	/* 执行一个 FORK 操作后,如果子进程的内存映像尚未建立起来,那么 NO_MAP 将被置位以阻止子进程运行 */
#define SENDING		    0x02	/* 进程正在试图发送一条消息 */
#define RECEIVING	    0x04	/* 进程正在试图接收一条消息 */

/* 进程权限定义 */
#define PROC_PRI_NONE	0	/* 表示该进程插槽未使用 */
#define PROC_PRI_TASK	1	/* 部分内核，即系统任务 */
#define PROC_PRI_SERVER	2	/* 内核之外的系统服务 */
#define PROC_PRI_USER	3	/* 用户进程 */
#define PROC_PRI_IDLE	4	/* 空闲进程，一个特殊的进程，当系统没有正在活动的进程时被运行 */

/* 对过程表地址操作的一些宏定义。 */
#define BEG_PROC_ADDR       (&proc_table[0])
#define END_PROC_ADDR       (&proc_table[NR_TASKS + NR_SERVERS + NR_PROCS])
#define END_TASK_ADDR       (&proc_table[NR_TASKS])
#define BEG_SERVER_ADDR     (&proc_table[NR_TASKS + NR_SERVERS])
#define BEG_USER_PROC_ADDR  (&proc_table[NR_TASKS + NR_SERVERS +LOW_USER])

/* 下面的这些宏能帮助我们快速做一些进程判断等简单的工作 */
#define NIL_PROC          ((Process_t *) 0)       /* 空进程指针 */
#define logic_nr_2_index(n) (NR_TASKS + n)
#define is_idle_hardware(n) ((n) == IDLE_TASK || (n) == HARDWARE)   /* 是空闲进程 或 硬件（特殊进程）？ */
#define is_ok_proc_nr(n)      ((unsigned) ((n) + NR_TASKS) < NR_PROCS + NR_TASKS + NR_SERVERS)   /* 是个合法的进程索引号？ */
#define is_ok_src_dest(n)   (is_ok_proc_nr(n) || (n) == ANY)                        /* 是个合法的发送或接收进程？ */
#define is_any_hardware(n)   ((n) == ANY || (n) == HARDWARE)                        /* 发送/接收进程是任何 或 硬件（特殊进程）？ */
#define is_sys_server(n)      ((n) == FS_PROC_NR || (n) == MM_PROC_NR || (n) == FLY_PROC_NR)      /* 是系统服务？ */
#define is_empty_proc(p)       ((p)->priority == PROC_PRI_NONE)             /* 是个空进程？ */
#define is_sys_proc(p)         ((p)->priority != PROC_PRI_USER)         /* 是个系统进程？ */
#define is_task_proc(p)        ((p)->priority == PROC_PRI_TASK)             /* 是个系统任务进程？ */
#define is_serv_proc(p)        ((p)->priority == PROC_PRI_SERVER)           /* 是个系统服务进程？ */
#define is_user_proc(p)        ((p)->priority == PROC_PRI_USER)             /* 是个用户进程？ */

/* 提供宏 proc_addr 是因为在C语言中下标不能为负数。在逻辑上,数组proc应从 -NR_TASKS到+NR_PROCS(因为用户进程从1开始)。
 * 但在C语言中下标必须从0开始,所以proc[0]指向进程表项下标最小的任务,其他也依次类推。为了更便于记录进程表项与
 * 进程之间的对应关系,我们可以使用
 * proc = proc_addr(n);
 * 将进程n的进程表项地址赋给rp,无论它是正还是负。
 */
#define proc_addr(n)      (p_proc_addr + NR_TASKS)[(n)]    /* 得到进程的指针 */
#define cproc_addr(n)     (&(proc_table + NR_TASKS)[(n)])  /* 得到进程的地址 */
/* 进程的虚拟地址转物理地址 */
#define proc_vir2phys(p, vir) \
    ((phys_bytes)(p)->map.base + (vir_bytes)(vir))

/* 进程表，记录系统的所有进程
 * 大小是
 */
EXTERN Process_t proc_table[NR_TASKS + NR_SERVERS + NR_PROCS];
EXTERN Process_t* p_proc_addr[NR_TASKS + NR_SERVERS + NR_PROCS]; /* 因为进程表的访问非常频繁,并且计算数组中的一个地址需要
                                                                  * 用到很慢的乘法操作, 所以使用一个指向进程表项的指针数组
                                                                  * p_proc_addr 来加快操作速度。 */

/* bill_proc指向正在对其CPU使用计费的进程。当一个用户进程调用文件系统,而文件系统正在运行
 * 时,curr_proc(在global.h中)指向文件系统进程,但是bill_proc将指向发出该调用的用户进程。因为文件系统使用的
 * CPU时间被作为调用者的系统时间来计费。
 */
EXTERN Process_t* bill_proc;

/* 两个数组ready_head和ready_tail用来维护调度队列。例如,ready_head[TASK_Q]指向就绪任务队列中的第一个进程。
 * 就绪进程队列一共分为三个
 * ready_head[TASK_QUEUE]：就绪系统任务队列
 * ready_head[SERVER_QUEUE]：就绪服务进程队列
 * ready_head[USER_QUEUE]：就绪用户进程队列
 * 再举个例子，我们需要拿到用户进程队列的第3个进程，则应该这么拿：ready_head[USER_QUEUE]->next_ready->next_ready，简单吧？
 */
EXTERN Process_t* ready_head[NR_PROC_QUEUE];
EXTERN Process_t* ready_tail[NR_PROC_QUEUE];

#endif //_FLYANX_PROCESS_H
