


#ifndef __TTY_H__
#define __TTY_H__

#include"type.h"
#include"struct_console.h"


#define TTY_IN_BYTES	256	/* tty input queue size */


/* TTY */
typedef struct s_tty
{
	char	in_buf[TTY_IN_BYTES];	/* TTY 输入缓冲区, 用于输出到屏幕 */
	char	*p_inbuf_head;		/* 指向缓冲区中下一个空闲位置 */
	char	*p_inbuf_tail;		/* 指向键盘任务应处理的键值 */
	int		inbuf_count;		/* 缓冲区中已经填充了多少 */

CONSOLE *	p_console;			/* 指向目前focus的控制台 */
}TTY;


#endif /* __TTY_H__ */
