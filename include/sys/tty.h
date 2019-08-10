
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
				tty.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef __TTY_H__
#define __TTY_H__

#include"type.h"
#include"console.h"


#define TTY_IN_BYTES		256	/* tty input queue size */
#define TTY_OUT_BUF_LEN		2	/* tty output buffer size */


/* TTY */
typedef struct s_tty
{
	u32		in_buf[TTY_IN_BYTES];	/* TTY 输入缓冲区, 用于输出到屏幕 */
	u32*	p_inbuf_head;		/* 指向缓冲区中下一个空闲位置 */
	u32*	p_inbuf_tail;		/* 指向键盘任务应处理的键值 */
	int		inbuf_count;		/* 缓冲区中已经填充了多少 */

	int		tty_caller;	// 向tty发送msg的进程,通常是task_fs
	int		tty_procnr;	// 请求数据的进程P
	void*	tty_req_buf;	// 进程P 用来存放读入字符的缓冲区的线性地址
	int		tty_left_cnt;	// 进程P 想要读入的字符数
	int		tty_trans_cnt;	// tty已经向进程P 传送的字符数

	CONSOLE *p_console;			/* 指向目前focus的控制台 */
}TTY;


#endif /* __TTY_H__ */
