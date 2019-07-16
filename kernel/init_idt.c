/*================================================================================================
File name:		kernel/init_idt.c
Description:	*初始化IDT
Copyright:		Chauncey Zhang
Date:		 	2019-7-16
===============================================================================================*/


#include"const.h"
#include"const_interrupt.h"
#include"global.h"
#include"type.h"
#include"func_proto.h"




/*======================================================================*
                           spurious_irq
	=================================================================
			默认的硬件中断服务程序(默认条件下,硬件中断是全部关闭的)
 *======================================================================*/
PUBLIC void spurious_irq(int irq)
{
        disp_str("spurious_irq: ");
        disp_int(irq);
        disp_str("\n");
}


/*======================================================================*
                           put_irq_handler
	=================================================================
			设置硬件中断服务程序,以代替默认的spurious_irq()
 *======================================================================*/
PUBLIC void put_irq_handler(int irq, irq_handler handler)
{
	disable_irq(irq);
	irq_table[irq] = handler;
}




/* 内部函数声明 */
PRIVATE	void init_8259A();
PRIVATE void init_idt_desc(unsigned char vector, u8 desc_type, int_handler handler, unsigned char privilege);

/*======================================================================*
                            init_idt
 *----------------------------------------------------------------------*
 初始化 IDT,GDT中的TSS和LDT
 *======================================================================*/
PUBLIC void init_idt()
{
	init_8259A();

	// 全部初始化成中断门(没有陷阱门)
	init_idt_desc(INT_VECTOR_DIVIDE,	DA_386IGate,
		      divide_error,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_DEBUG,		DA_386IGate,
		      single_step_exception,	PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_NMI,		DA_386IGate,
		      nmi,			PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_BREAKPOINT,	DA_386IGate,
		      breakpoint_exception,	PRIVILEGE_USER);

	init_idt_desc(INT_VECTOR_OVERFLOW,	DA_386IGate,
		      overflow,			PRIVILEGE_USER);

	init_idt_desc(INT_VECTOR_BOUNDS,	DA_386IGate,
		      bounds_check,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_INVAL_OP,	DA_386IGate,
		      inval_opcode,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_COPROC_NOT,	DA_386IGate,
		      copr_not_available,	PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_DOUBLE_FAULT,	DA_386IGate,
		      double_fault,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_COPROC_SEG,	DA_386IGate,
		      copr_seg_overrun,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_INVAL_TSS,	DA_386IGate,
		      inval_tss,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_SEG_NOT,	DA_386IGate,
		      segment_not_present,	PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_STACK_FAULT,	DA_386IGate,
		      stack_exception,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_PROTECTION,	DA_386IGate,
		      general_protection,	PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_PAGE_FAULT,	DA_386IGate,
		      page_fault,		PRIVILEGE_KRNL);

	init_idt_desc(INT_VECTOR_COPROC_ERR,	DA_386IGate,
		      copr_error,		PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ0 + 0,      DA_386IGate,
                      hwint00,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ0 + 1,      DA_386IGate,
                      hwint01,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ0 + 2,      DA_386IGate,
                      hwint02,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ0 + 3,      DA_386IGate,
                      hwint03,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ0 + 4,      DA_386IGate,
                      hwint04,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ0 + 5,      DA_386IGate,
                      hwint05,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ0 + 6,      DA_386IGate,
                      hwint06,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ0 + 7,      DA_386IGate,
                      hwint07,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ8 + 0,      DA_386IGate,
                      hwint08,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ8 + 1,      DA_386IGate,
                      hwint09,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ8 + 2,      DA_386IGate,
                      hwint10,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ8 + 3,      DA_386IGate,
                      hwint11,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ8 + 4,      DA_386IGate,
                      hwint12,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ8 + 5,      DA_386IGate,
                      hwint13,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ8 + 6,      DA_386IGate,
                      hwint14,                  PRIVILEGE_KRNL);

        init_idt_desc(INT_VECTOR_IRQ8 + 7,      DA_386IGate,
                      hwint15,                  PRIVILEGE_KRNL);

	/* 系统调用软件中断INT 90H */
	init_idt_desc(INT_VECTOR_SYS_CALL,	DA_386IGate,
		      sys_call,			PRIVILEGE_USER);
}


/*======================================================================*
                            init_8259A
 *======================================================================*/
PRIVATE void init_8259A()
{
	out_byte(INT_M_CTL,	0x11);			// Master 8259, ICW1.
	out_byte(INT_S_CTL,	0x11);			// Slave  8259, ICW1.
	out_byte(INT_M_CTLMASK,	INT_VECTOR_IRQ0);	// Master 8259, ICW2. 设置 '主8259' 的中断入口地址为 0x20.
	out_byte(INT_S_CTLMASK,	INT_VECTOR_IRQ8);	// Slave  8259, ICW2. 设置 '从8259' 的中断入口地址为 0x28
	out_byte(INT_M_CTLMASK,	0x4);			// Master 8259, ICW3. IR2 对应 '从8259'.
	out_byte(INT_S_CTLMASK,	0x2);			// Slave  8259, ICW3. 对应 '主8259' 的 IR2.
	out_byte(INT_M_CTLMASK,	0x1);			// Master 8259, ICW4.
	out_byte(INT_S_CTLMASK,	0x1);			// Slave  8259, ICW4.

	/* 禁止所有中断 */
	out_byte(INT_M_CTLMASK,	0xFF);	// Master 8259, OCW1. 
	out_byte(INT_S_CTLMASK,	0xFF);	// Slave  8259, OCW1.

	for (int i = 0; i < NR_IRQ; i++) 
	{
		irq_table[i] = spurious_irq;
	}
}


/*======================================================================*
                             init_idt_desc
 *----------------------------------------------------------------------*
 初始化 386 中断门
 *======================================================================*/
PRIVATE void init_idt_desc(
	unsigned char vector,
	u8 desc_type,
	int_handler handler,
	unsigned char privilege)
{
	GATE *	p_gate	= &idt[vector];
	u32	base	= (u32)handler;
	p_gate->offset_low	= base & 0xFFFF;
	p_gate->selector	= SELECTOR_KERNEL_CS;
	p_gate->dcount		= 0;
	p_gate->attr		= desc_type | (privilege << 5);
	p_gate->offset_high	= (base >> 16) & 0xFFFF;
}
