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


/* 辅助函数 */
/* init_descriptor */
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
	for(int i=0;i<NR_TASKS+NR_PROCS;i++)
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
	

		u8              privilege;
        u8              rpl;
        int             eflags;
		int				priority;
	/* 进程表初始化 */
	for (int i = 0; i < NR_TASKS + NR_PROCS; i++) 
	{
				if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
						priority  =	15;
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
						priority  =	5;
                }

		/* 填充各个进程表的成员 */
		strcpy(p_proc->name, p_task->name);	// name of the process
		p_proc->pid = i;						// pid

		p_proc->ldt_sel = selector_ldt;


		/* 填充各个进程表中的LDT selector */
		/* 填充进程的代码段选择子 */
		memcpy(&p_proc->ldts[0],
				&gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;

		/* 填充进程的数据段选择子 */
		memcpy(&p_proc->ldts[1],
				&gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;

		/* 填充进程表中的segment selector
		 * cs->ldt[0], ds/es/ss/fs->ldt[1], gs->显存 */
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | rpl;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | rpl;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | rpl;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | rpl;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)	| rpl;

		/* 填充进程表中进程的 EIP 和 ESP (ring1-3) */
		p_proc->regs.eip	= (u32)p_task->initial_eip;
		p_proc->regs.esp 	= (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		p_proc->nr_tty 		= 0;		/* 所有进程默认使用tty0 */

		/* 填充IPC相关 */
 		p_proc->p_flags 	= 0;	/* ready */
		p_proc->p_msg 		= 0;
		p_proc->p_recvfrom 	= NO_TASK;
		p_proc->p_sendto 	= NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending 	= 0;
		p_proc->next_sending = 0;

		/* 进程调用,进程优先级 */
		p_proc->ticks = p_proc->priority = priority;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

		/* user process take up TTY 1 */
        proc_table[2].nr_tty = 1;
        proc_table[3].nr_tty = 1;
        proc_table[4].nr_tty = 1;


	/* 准备进程调度 */
	k_reenter 		= 0;
	ticks 			= 0;
	p_proc_ready	= proc_table;	/* ready process */


        /* 准备时钟中断 */
		init_clock();

		/* 准备键盘中断 */
    	init_keyboard();


	/* ring0->ring1,开始执行任务/进程 */
	restart(); /* 执行p_proc_ready->指向的进程 */


	/* WARNING: You can't come here! */
	while(1)
	{
		/* nop */
	}
}
