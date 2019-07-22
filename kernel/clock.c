/*================================================================================================
File name:		kernel/clock.c
Description:	*定义时钟中断处理程序
				*定义其他与时钟相关的底层函数
Copyright:		Chauncey Zhang
Date:		 	2019-7-14
===============================================================================================*/


#include"const.h"
#include"global.h"
#include"func_proto.h"
#include"const_interrupt.h"


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

	/* 进程调度 */
	schedule();
}


/*======================================================================*
                              milli_delay
	===================================================================
						安装10ms的数量级进行延迟
 *======================================================================*/
PUBLIC void milli_delay(int milli_sec)
{
        int t = ticks;	/* 此处直接引用ticks,而不是使用get_ticks()系统调用,是为了时钟精确性 */

        while(((ticks - t) * 1000 / HZ) < milli_sec) 
		{
			/* nop */
		}
}


/*======================================================================*
                           init_clock
 *======================================================================*/
PUBLIC void init_clock()
{
        /* 初始化 8253 PIT */
        out_byte(TIMER_MODE, RATE_GENERATOR);
        out_byte(TIMER0, (u8) (TIMER_FREQ/HZ) );	/* 设置8253每隔1ms产生一次中断 */
        out_byte(TIMER0, (u8) ((TIMER_FREQ/HZ) >> 8));

        put_irq_handler(CLOCK_IRQ, clock_handler);    /* 设定时钟中断处理程序 */
        enable_irq(CLOCK_IRQ);                        /* 让8259A可以接收时钟中断 */
}


/*======================================================================*
                           sys_get_ticks
	==================================================================
		系统调用get_tichs()->INT中断->sys_call()->sys_get_ticks()
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}