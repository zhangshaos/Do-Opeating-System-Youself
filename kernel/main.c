/*================================================================================================
File name:		kernel/main.c
Description:	*kernel_main():初始化个进程的PCB,跳转到任务进程
				*定义三个任务
Copyright:		Chauncey Zhang
Date:		 	2019-7-14
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"


/*======================================================================*
                            kernel_main
	===================================================================
							初始化化各进程的PCB
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	/* 注意task Stack是一整块,由p_proc->regs.esp分隔开 */
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	
	for (int i = 0; i < NR_TASKS; i++) 
	{
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;						// pid

		p_proc->ldt_sel = selector_ldt;

		/* 填充各个PCB中的LDT selector */
		memcpy(&p_proc->ldts[0],
				&gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;

		memcpy(&p_proc->ldts[1],
				&gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;

		/* 填充PCB中的segment selector */
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)	| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)	| RPL_TASK;

		/* 填充PCB中进程的 EIP 和 ESP (ring1-3) */
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

	k_reenter = 0;
	ticks = 0;

	/* 准备进程调度 */
	p_proc_ready	= proc_table;

        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

		/* 打开时钟中断 */
        put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

	/* ring0->ring1,开始执行任务 */
	restart();

	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n!!!Task Over!!!\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"); /* in common, you can't come on here. */
	while(1)
	{
		/* nop */
	}
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	int i = 0;
	while (1) {
		disp_str("A.");
		milli_delay(1000);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int i = 0x1000;
	while(1){
		disp_str("B.");
		milli_delay(1000);
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	int i = 0x2000;
	while(1){
		disp_str("C.");
		milli_delay(1000);
	}
}
