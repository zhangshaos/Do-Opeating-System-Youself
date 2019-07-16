/*================================================================================================
File name:		kernel/main.c
Description:	*内核的初始化
				*进程表的初始化
				*从内核跳向进程
Copyright:		Chauncey Zhang
Date:		 	2019-7-16
===============================================================================================*/


#include"type.h"
#include"const.h"
#include"global.h"
#include"struct_descript.h"
#include"const_interrupt.h"
#include"func_proto.h"




/*======================================================================*
                           init_descriptor
 *======================================================================*/
PRIVATE void init_descriptor(DESCRIPTOR * p_desc, u32 base, u32 limit, u16 attribute)
{
	p_desc->limit_low		= limit & 0x0FFFF;			// 段界限 1		(2 字节)
	p_desc->base_low		= base & 0x0FFFF;			// 段基址 1		(2 字节)
	p_desc->base_mid		= (base >> 16) & 0x0FF;		// 段基址 2		(1 字节)
	p_desc->attr1			= attribute & 0xFF;			// 属性 1
	p_desc->limit_high_attr2	= ((limit >> 16) & 0x0F) |
						(attribute >> 8) & 0xF0;		// 段界限 2 + 属性 2
	p_desc->base_high		= (base >> 24) & 0x0FF;		// 段基址 3		(1 字节)
}


/* 段选择符 -> 段基地址 */
static inline u32 seg2phys(u16 seg)
{
	DESCRIPTOR* p_dest = &gdt[seg >> 3];

	return (p_dest->base_high << 24) | (p_dest->base_mid << 16) | (p_dest->base_low);
}


/* 基址 + 偏移 → 物理地址 */
static inline u32 vir2phys(u32 seg_base, void *vir)
{
	return (u32)(((u32)seg_base) + (u32)(vir));
}




/*======================================================================*
                            cstart
				1.将gdt从loader中移到kernel中,
				2.初始化idt,gdt中的tss和idt
 *======================================================================*/
PUBLIC void cstart()
{
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n-----\"cstart\" begins-----\n");

	/* 将 LOADER 中的 GDT 复制到新的 GDT 中 */
	memcpy(&gdt,							/* New GDT */
	       (void*)(*((u32*)(&gdt_ptr[2]))),	/* Base  of Old GDT */
	       *((u16*)(&gdt_ptr[0])) + 1	  	/* Scale of Old GDT */
		);

	/* 创建新的gdt_ptr;
	   gdt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sgdt/lgdt 的参数。*/
	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
	u32* p_gdt_base  = (u32*)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
	*p_gdt_base  = (u32)&gdt;

	/* 创建新的idt_ptr,
	   idt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sidt/lidt 的参数。*/
	u16* p_idt_limit = (u16*)(&idt_ptr[0]);
	u32* p_idt_base  = (u32*)(&idt_ptr[2]);
	*p_idt_limit = IDT_SIZE * sizeof(GATE) - 1;
	*p_idt_base  = (u32)&idt;

	
	/* 初始化 IDT */
	init_idt();


	/* 初始化 TSS */
	memset(&tss, 0, sizeof(tss));
	tss.ss0		= SELECTOR_KERNEL_DS;
	tss.iobase	= sizeof(tss);		/* 没有I/O许可位图 */


	/* 初始化 GDT 中的TSS描述符 */
	init_descriptor(&gdt[INDEX_TSS],
			vir2phys(seg2phys(SELECTOR_KERNEL_DS), &tss),
			sizeof(tss) - 1,
			DA_386TSS);


	/* 初始化 GDT 中的LDT描述符 */
	u16 selector_ldt = INDEX_LDT_FIRST << 3;
	for(int i=0;i<NR_TASKS;i++)
	{
		init_descriptor(&gdt[selector_ldt>>3],
				vir2phys(seg2phys(SELECTOR_KERNEL_DS),
					proc_table[i].ldts),
				LDT_SIZE * sizeof(DESCRIPTOR) - 1,
				DA_LDT);
		selector_ldt += 1 << 3;
	}

	disp_str("-----\"cstart\" finished-----\n");
}












/*======================================================================*
                            kernel_main
	===================================================================
							初始化化进程表
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	/* 注意task Stack是一整块,由p_proc->regs.esp分隔开 */
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16			selector_ldt	= SELECTOR_LDT_FIRST;
	

	/* 进程表初始化 */
	for (int i = 0; i < NR_TASKS; i++) 
	{
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;						// pid

		p_proc->ldt_sel = selector_ldt;

		/* 填充各个进程表中的LDT selector */
		memcpy(&p_proc->ldts[0],
				&gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;

		memcpy(&p_proc->ldts[1],
				&gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;

		/* 填充进程表中的segment selector */
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)	| RPL_TASK;

		/* 填充进程表中进程的 EIP 和 ESP (ring1-3) */
		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].ticks = proc_table[0].priority = 1500;	/* 15秒 */
	proc_table[1].ticks = proc_table[1].priority =  500;
	proc_table[2].ticks = proc_table[2].priority =  300;


	/* 准备进程调度 */
	k_reenter 		= 0;
	ticks 			= 0;
	p_proc_ready	= proc_table;	/* ready process */


        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

		/* 打开时钟中断,让时钟中断选择 ready process */
        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */


	/* ring0->ring1,开始执行任务/进程 */
	restart(); /* 执行p_proc_ready->指向的进程 */


	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n!!!Task Over!!!\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); /* in common, you can't come on here. */
	while(1)
	{
		/* nop */
	}
}
