/* Copyright (C) 2007 Free Software Foundation, Inc. 
 * See the copyright notice in the file /usr/LICENSE.
 * Created by flyan on 2020/2/23.
 * QQ: 1341662010
 * QQ-Group:909830414
 * gitee: https://gitee.com/flyanh/
 *
 * protect.c包含与Intel处理器保护模式相关的例程。
 *
 * 本文件的入口点是：
 *  - protect_init          保护模式初始化
 *  - init_segment_desc     初始化一个段描述符
 *  - seg2phys              由段名求其在内存中的物理地址
 */

#include "kernel.h"
#include "protect.h"

/* 全局描述符表GDT */
PUBLIC SegDescriptor_t gdt[GDT_SIZE];
/* 中断描述符表IDT */
PRIVATE Gate_t idt[IDT_SIZE];
/* 任务状态段TSS(Task-State Segment) */
PUBLIC Tss_t tss;

/*=========================================================================*
 *				protect_init				   *
 *				保护模式初始化
 *=========================================================================*/
PUBLIC void protect_init(void){

    /* 首先，将 LOADER 中的 GDT 拷贝到内核中新的 GDT 中  */
    phys_copy(*((u32_t *)vir2phys(&gdt_ptr[2])),            // src:LOADER中旧的GDT基地址
            vir2phys(&gdt),                                // dest:新的GDT基地址
            *((u16_t*)vir2phys(&gdt_ptr[0])) + 1           // size:旧GDT的段界限 + 1
            );
    /* 算出新 GDT 的基地址和界限，设置新的 gdt_ptr */
    u16_t* p_gdt_limit = (u16_t*)vir2phys(&gdt_ptr[0]);
    u32_t* p_gdt_base = (u32_t*)vir2phys(&gdt_ptr[2]);
    *p_gdt_limit = GDT_SIZE * DESCRIPTOR_SIZE - 1;
    *p_gdt_base = vir2phys(&gdt);
    /* 算出IDT的基地址和界限，设置新的 idt_ptr */
    u16_t* p_idt_limit = (u16_t*)vir2phys(&idt_ptr[0]);
    u32_t* p_idt_base = (u32_t*)vir2phys(&idt_ptr[2]);
    *p_idt_limit = IDT_SIZE * sizeof(Gate_t) - 1;
    *p_idt_base = vir2phys(&idt);

    /* 初始化任务状态段TSS，并为处理器寄存器和其他任务切换时应保存的信息提供空间。
     * 我们只使用了某些域的信息，这些域定义了当发生中断时在何处建立新堆栈。
     * 下面init_seg_desc的调用保证它可以用GDT进行定位。
     */
    memset((void*)vir2phys(&tss), 0, sizeof(tss));
    tss.ss0 = SELECTOR_KERNEL_DS;
    init_segment_desc(&gdt[TSS_INDEX], vir2phys(&tss), sizeof(tss) - 1, DA_386TSS);
    tss.iobase = sizeof(tss);           /* 空 I/O 位图 */

}

/*=========================================================================*
 *				init_segment_desc				   *
 *				初始化段描述符
 *=========================================================================*/
PUBLIC void init_segment_desc(
        SegDescriptor_t *p_desc,
        phys_bytes base,
        phys_bytes limit,
        u16_t attribute
)
{
    /* 初始化一个数据段描述符 */
    p_desc->limit_low	= limit & 0x0FFFF;         /* 段界限 1		(2 字节) */
    p_desc->base_low	= base & 0x0FFFF;          /* 段基址 1		(2 字节) */
    p_desc->base_middle	= (base >> 16) & 0x0FF;     /* 段基址 2		(1 字节) */
    p_desc->access		= attribute & 0xFF;         /* 属性 1 */
    p_desc->granularity = ((limit >> 16) & 0x0F) |  /* 段界限 2 + 属性 2 */
                          ((attribute >> 8) & 0xF0);
    p_desc->base_high	= (base >> 24) & 0x0FF;     /* 段基址 3		(1 字节) */
}

/*=========================================================================*
 *				init_gate				   *
 *				初始化一个 386 中断门
 *=========================================================================*/
PRIVATE void init_gate(
        u8_t vector,
        u8_t desc_type,
        int_handler_t  handler,
        u8_t privilege
)
{
    // 得到中断向量对应的门结构
    Gate_t* p_gate = &idt[vector];
    // 取得处理函数的基地址
    u32_t base_addr = (u32_t)handler;
    // 一一赋值
    p_gate->offset_low = base_addr & 0xFFFF;
    p_gate->selector = SELECTOR_KERNEL_CS;
    p_gate->dcount = 0;
    p_gate->attr = desc_type | (privilege << 5);
#if _WORD_SIZE == 4
    p_gate->offset_high = (base_addr >> 16) & 0xFFFF;
#endif
}

/*=========================================================================*
 *				seg2phys				   *
 *		由段名求其在内存中的物理地址
 *=========================================================================*/
PUBLIC phys_bytes seg2phys(U16_t seg)
{
    SegDescriptor_t* p_dest = &gdt[seg >> 3];
    return (p_dest->base_high << 24 | p_dest->base_middle << 16 | p_dest->base_low);
}

