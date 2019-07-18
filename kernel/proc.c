/*================================================================================================
File name:		kernel/proc.c
Description:	*进程调度
				*系统调用定义
Copyright:		Chauncey Zhang
Date:		 	2019-7-14
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "proto.h"

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
			if (p->ticks > greatest_ticks) {
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

		/* 如果所有进程都结束了, 重新跑一遍 */
		if (0 == greatest_ticks) 
		{
			for (p = proc_table; p < proc_table+NR_TASKS+NR_PROCS; p++) {
				p->ticks = p->priority;
			}
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

