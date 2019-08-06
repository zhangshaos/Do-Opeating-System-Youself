/*================================================================================================
File name:		kernel/proc.c
Description:	*定义进程调度函数
Copyright:		Chauncey Zhang
Date:		 	2019-7-14
===============================================================================================*/


#include"const.h"
#include"struct_proc.h"
#include"global.h"
#include"func_proto.h"
#include"stdio.h"


/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p;
	int	 greatest_ticks = 0;

	while (0 == greatest_ticks) 
	{
		/* 寻找剩余ticks(优先级)最多的进程 */
		for (p = proc_table; p < proc_table+NR_TASKS+NR_PROCS; p++) 
		{
			/* 只选择"ready"的进程 */
			if(0 == p->p_status)
			{
				if (p->ticks > greatest_ticks)
				{
					greatest_ticks = p->ticks;
					p_proc_ready = p;
				}
			}
		}

		/* 如果所有进程都结束了, 重新跑一遍 */
		if (0 == greatest_ticks) 
		{
			for (p = proc_table; p < proc_table+NR_TASKS+NR_PROCS; p++) 
			{
				if(0 == p->p_status)
				{
					p->ticks = p->priority;
				}
			}
		}
	}
}






/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	int fd = open("/blah", O_CREAT);
	printf("fd: %d\n", fd);
	close(fd);
	spin("TestA");
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int i = 0x1000;
	while(1){
		printf("B.");
		milli_delay(1000);
	}
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{
	int i = 0x2000;
	while(1){
		printf("C.");
		milli_delay(1000);
	}
}