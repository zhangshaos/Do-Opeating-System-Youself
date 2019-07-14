/*================================================================================================
File name:		kernel/clock.c
Description:	*定义和时钟有关的高层函数
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
                           clock_handler
 *======================================================================*/
PUBLIC void clock_handler(int irq)
{
	ticks++;
	p_proc_ready->ticks--;	/* 减少正在运行的进程的剩余ticks */

	if (k_reenter != 0) /* 发生中断重入的时候, 不进行进程调度 */
	{
		return;
	}

	if (p_proc_ready->ticks > 0) /* 当进程的ticks用完后, 才进行调度 */
	{
		return;
	}

	schedule();

}

/*======================================================================*
                              milli_delay
	===================================================================
						安装10ms的数量级进行延迟
 *======================================================================*/
PUBLIC void milli_delay(int milli_sec)
{
        int t = get_ticks();

        while(((get_ticks() - t) * 1000 / HZ) < milli_sec) 
		{
			/* nop */
		}
}

