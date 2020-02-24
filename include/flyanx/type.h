/* Copyright (C) 2007 Free Software Foundation, Inc.
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2019/11/9.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * 该文件包含了许多重要的类型定义以及相关的数量值
 * 该文件最重要的定义是Message，其定义了以消息传递的系统体系中最重要的
 * 消息结构体。
 */

#ifndef _FLYANX_TYPE_H
#define _FLYANX_TYPE_H

#ifndef _FLYANX_TYPES_H
#include <sys/types.h>
#endif

/* 虚拟内存块，一个块在INTEL的Flyanx中是256个字节 */
typedef unsigned int vir_clicks;
/* 物理地址（字节） */
typedef unsigned long phys_bytes;
/* 物理内存块，一个块在INTEL的Flyanx中是256个字节 */
typedef unsigned int phys_clicks;

#if (CHIP == INTEL)
/* 虚拟地址（字节） */
typedef unsigned int vir_bytes;
#endif


/* 与消息类型有关的定义。 */
#define M1                 1        /* 消息类型1：消息域使用mess_union1 */
#define M3                 3        /* 同上 */
#define M4                 4        /* 同上 */
#define M3_STRING         15        /* 消息类型3携带字符串的长度 */

/* 定义6种消息域将使得更易于在不同的体系结构上重新编译。 */
typedef struct {int m1i1, m1i2, m1i3; char *m1p1, *m1p2, *m1p3;} mess_union1;
typedef struct {int m2i1, m2i2, m2i3; long m2l1, m2l2; char *m2p1;} mess_union2;
typedef struct {int m3i1, m3i2; char *m3p1; char m3ca1[M3_STRING];} mess_union3;
typedef struct {long m4l1, m4l2, m4l3, m4l4, m4l5;} mess_union4;
typedef struct {char m5c1, m5c2; int m5i1, m5i2; long m5l1, m5l2, m5l3;}mess_union5;
typedef struct {int m6i1, m6i2, m6i3; long m6l1; sighandler_t m6f1;} mess_union6;

/* 消息，Flyanx中的进程通信的根本，同时也是客户端和服务端通信的根本
 * 此数据结构来源自MINIX
 */
typedef struct message_s{
    int source;         /* 谁发送的消息 */
    int type;           /* 消息的类型，用于判断告诉对方意图 */
    union {             /* 消息域，一共可以是六种消息域类型之一 */
        mess_union1 m_u1;
        mess_union2 m_u2;
        mess_union3 m_u3;
        mess_union4 m_u4;
        mess_union5 m_u5;
        mess_union6 m_u6;
    } m_u;
} Message_t;

/* 消息常用宏定义 */
#define MESSAGE_SIZE    (sizeof(Message_t))   /* 一个消息的字节大小 */
#define NIL_MESSAGE     ((Message_t *) 0)     /* 空消息 */

/* 以下定义提供了消息中消息域有用成员的简短名称。 */
/* 消息域1中的消息属性 */
#define m1_i1   m_u.m_u1.m1i1
#define m1_i2   m_u.m_u1.m1i2
#define m1_i3   m_u.m_u1.m1i3
#define m1_p1   m_u.m_u1.m1p1
#define m1_p2   m_u.m_u1.m1p2
#define m1_p3   m_u.m_u1.m1p3

/* 消息域2中的消息属性 */
#define m2_i1   m_u.m_u2.m2i1
#define m2_i2   m_u.m_u2.m2i2
#define m2_i3   m_u.m_u2.m2i3
#define m2_l1   m_u.m_u2.m2l1
#define m2_l2   m_u.m_u2.m2l2
#define m2_p1   m_u.m_u2.m2p1

/* 消息域3中的消息属性 */
#define m3_i1   m_u.m_u3.m3i1
#define m3_i2   m_u.m_u3.m3i2
#define m3_p1   m_u.m_u3.m3p1
#define m3_ca1  m_u.m_u3.m3ca1

/* 消息域4中的消息属性 */
#define m4_l1   m_u.m_u4.m4l1
#define m4_l2   m_u.m_u4.m4l2
#define m4_l3   m_u.m_u4.m4l3
#define m4_l4   m_u.m_u4.m4l4
#define m4_l5   m_u.m_u4.m4l5

/* 消息域5中的消息属性 */
#define m5_c1   m_u.m_u5.m5c1
#define m5_c2   m_u.m_u5.m5c2
#define m5_i1   m_u.m_u5.m5i1
#define m5_i2   m_u.m_u5.m5i2
#define m5_l1   m_u.m_u5.m5l1
#define m5_l2   m_u.m_u5.m5l2
#define m5_l3   m_u.m_u5.m5l3

/* 消息域6中的消息属性 */
#define m6_i1   m_u.m_u6.m6i1
#define m6_i2   m_u.m_u6.m6i2
#define m6_i3   m_u.m_u6.m6i3
#define m6_l1   m_u.m_u6.m6l1
#define m6_f1   m_u.m_u6.m6f1

/* 内存映像
 * 这个结构能够描述一个内存块信息
 */
typedef struct memory_map{
    phys_bytes base;    /* 这块内存的基地址 */
    phys_bytes size;    /* 这块内存有多大？ */
} MemoryMap_t;

/* 启动参数 */
typedef struct boot_params_s {
    u32_t memory_size;          /* 内存大小 */
    phys_bytes kernel_file;     /* 内核所在绝对物理地址 */
} BootParams_t;

#endif //_FLYANX_TYPE_H
