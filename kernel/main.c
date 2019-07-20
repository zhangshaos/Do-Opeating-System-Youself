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
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


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
	
        u8              privilege;
        u8              rpl;
        int             eflags;
	for (int i = 0; i < NR_TASKS+NR_PROCS; i++)
	{
                if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
                }

		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;						// pid

		p_proc->ldt_sel = selector_ldt;

		/* 填充各个PCB中的LDT selector */
		memcpy(&p_proc->ldts[0],
				&gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;

		memcpy(&p_proc->ldts[1],
				&gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;

		/* 填充PCB中的segment selector */
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		/* 填充PCB中进程的 EIP 和 ESP (ring1-3) */
		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		p_proc->nr_tty = 0;		/* 所有进程默认使用tty0 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].ticks = proc_table[0].priority = 	500;	/* 5秒 */
	proc_table[1].ticks = proc_table[1].priority =  500;
	proc_table[2].ticks = proc_table[2].priority =  500;
	proc_table[3].ticks = proc_table[3].priority =  500;

        proc_table[1].nr_tty = 1;
        proc_table[2].nr_tty = 1;
        proc_table[3].nr_tty = 1;

	k_reenter = 0;
	ticks = 0;

	/* 准备进程调度 */
	p_proc_ready	= proc_table;

	/* 准备时钟中断 */
	init_clock();
	/* 准备键盘中断 */
    init_keyboard();


	/* ring0->ring1,开始执行任务 */
	restart();

	disp_str("\n!!!Process Over!!!\n"); /* in common, you can't come on here. */
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
		printf("<Ticks:%x>", get_ticks());	/* 进程A的两次printf之间大概100(0x64)个ticks,每个ticks10ms,即1000ms(1s) */
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
		printf("B");
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
		printf("C");
		milli_delay(1000);
	}
}
