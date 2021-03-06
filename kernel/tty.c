/*************************************************************************//**
 *****************************************************************************
 * @file   kernel/tty.c
 * @brief  The terminal driver.
 *
 * As a common driver, TTY accepts these MESSAGEs:
 *   - DEV_OPEN
 *   - DEV_READ
 *   - DEV_WRITE
 *
 * Besides, it accepts the other two types of MESSAGE from clock_handler() and
 * a PROC (who is not FS):
 *
 *   - MESSAGE from clock_handler(): HARD_INT
 *      - Every time clock interrupt occurs, the clock handler will check whether
 *        any key has been pressed. If so, it'll invoke inform_int() to wake up
 *        TTY. It is a special message because it is not from a process -- clock
 *        handler is not a process.
 *
 *   - MESSAGE from a PROC: TTY_WRITE
 *      - TTY is a driver. In most cases MESSAGE is passed from a PROC to FS then
 *        to TTY. For some historical reason, PROC is allowed to pass a TTY_WRITE
 *        MESSAGE directly to TTY. Thus a PROC can write to a tty directly.
 *
 * @note   Do not get confused by these function names:
 *           - tty_dev_read() vs tty_do_read()
 *             - tty_dev_read() reads chars from keyboard buffer
 *             - tty_do_read() handles DEV_READ message
 *           - tty_dev_write() vs tty_do_write() vs tty_write()
 *             - tty_dev_write() returns chars to a process waiting for input
 *             - tty_do_write() handles DEV_WRITE message
 *             - tty_write() handles TTY_WRITE message
 *
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

PRIVATE void	init_tty(TTY* tty);
PRIVATE void	tty_dev_read(TTY* tty);
PRIVATE void	tty_dev_write(TTY* tty);
PRIVATE void	tty_do_read(TTY* tty, MESSAGE* msg);
PRIVATE void	tty_do_write(TTY* tty, MESSAGE* msg);
PRIVATE void	put_key(TTY* tty, u32 key);

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
	TTY*	p_tty;
	MESSAGE	msg;

	/* 允许键盘中断 */
	init_keyboard();

	/* 初始化所有p_tty */
	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) 
	{
		init_tty(p_tty);
	}

	select_console(0);

	while (1) 
	{
		// 轮训各TTY,响应用户键盘
		// @1:回显用户输入
		// @2:将键盘输入数据输送给读取键盘输入的进程P
		for (p_tty = TTY_FIRST; p_tty < TTY_END; p_tty++) 
		{
			// LOG_RECORD(" tty-%d:dev_read&write ",p_tty-tty_table);
			// do {
				tty_dev_read(p_tty);	//将键盘扫描码(没有扫描码时忙等出现)转为key并输入盘p_tty的输入缓冲区
				LOG_RECORD("\ntty%d:scan code transfered\n",p_tty-tty_table);
				tty_dev_write(p_tty);	//将p_tty的缓冲区数据写入读取tty数据进程P中	//没有进程P 会发生什么?
			// } while (p_tty->inbuf_count);
			// 为什么要使用do-while? 不使用循环就行把.
			// tty_dev_write()调用后,inbuf_count = 0;
		}

		// 接受直接读写tty的msg
		// @1:显示进程输出屏幕的数据
		// @2:让读取键盘输入的进程p 等用户键入数据完成.
		send_recv(RECEIVE, ANY, &msg);

		int src = msg.source;
		assert(src != TASK_TTY);

		TTY* ptty = &tty_table[msg.DEVICE];

		switch (msg.type) 
		{
		case DEV_OPEN:	//do nothing, just return.
			reset_msg(&msg);
			msg.type = SYSCALL_RET;
			send_recv(SEND, src, &msg);
			break;
		case DEV_READ:	//其他进程想要读tty设备(读取键盘)
			// @1:使用msg 填充struct tty 中关于读取数据的进程P 的数据结构
			// @2:告诉fs系统,挂起read tty 的进程P,等tty读完键盘...
			tty_do_read(ptty, &msg);
			break;
		case DEV_WRITE:	//其他进程想要读写tty设备(输出到屏幕)
			// 将msg 传送过来的字符,显示出来.
			tty_do_write(ptty, &msg);
			break;
		case HARD_INT:
			/**
			 * waked up by clock_handler -- a key was just pressed
			 * @see clock_handler() inform_int()
			 */
			LOG_RECORD("\nhard int to tty\n");
			key_pressed = 0;
			continue;	//继续循环tty_dev_read && tty_dev_write
		default:
			dump_msg("TTY::unknown msg", &msg);
			break;
		}
	}
}

/*======================================================================*
			   init_tty
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	p_tty->tty_caller = p_tty->tty_procnr = NO_TASK;
	p_tty->tty_req_buf = (void*)0;
	p_tty->tty_left_cnt = 0;
	p_tty->tty_trans_cnt = 0;

	/* 初始化屏幕(console) */
	init_screen(p_tty);
}

/*****************************************************************************
 *                                in_process
 *****************************************************************************/
/**
 * 解析自定义的键key:
 * 如果是可打印字符,则将其放在 @param:p_tty 的打印缓冲中,等待打印
 * 如果是功能按键,则执行相应功能
 *****************************************************************************/
PUBLIC void in_process(TTY* p_tty, u32 key)
{

        if (!(key & FLAG_EXT))	/* 如果是可打印字符, 则将其打印出来 */
		{
			put_key(p_tty, key);
        }
        else
		{
            int raw_code = key & MASK_RAW;
        	switch(raw_code)
			{
            case ENTER:
				put_key(p_tty, '\n');
				break;
            case BACKSPACE:
				put_key(p_tty, '\b');
				break;
            case UP:
                if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
				{
					scroll_screen(p_tty->p_console, SCR_DN);        
				}
				break;
			case DOWN:
				if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
				{
					scroll_screen(p_tty->p_console, SCR_UP);
				}
				break;
			case F1:
			case F2:
			case F3:
			case F4:	/* 好像只要按下alt + f4, bochs就直接卡住了... */
			case F5:
			case F6:
			case F7:
			case F8:
			case F9:
			case F10:
			case F11:
			case F12:
				select_console(raw_code - F4);	/* I only need f4-f6 */
				/* Alt + F4 */
				if ((key & FLAG_ALT_L) || (key & FLAG_ALT_R))
				{
					/* fuck Alt! in my PC, Alt + Fx is a awful bug! 
					Alt + Fx : not cpatured...
					Alt + F4 : kill the bochs!!! */
				}
				break;
            default:
                break;
            }
        }
}

/*======================================================================*
			      			put_key
	================================================================
				将key放置到tty.in_buf[]中, 等待输出到屏幕
*======================================================================*/
PRIVATE void put_key(TTY* p_tty, u32 key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) 
	{
		*(p_tty->p_inbuf_head) = key;
		p_tty->p_inbuf_head++;
		if (p_tty->p_inbuf_head == p_tty->in_buf + TTY_IN_BYTES)
		{
			p_tty->p_inbuf_head = p_tty->in_buf;
		}
		p_tty->inbuf_count++;
	}
}


/*****************************************************************************
 				将键盘扫描码转为key并输入盘p_tty的输入缓冲区
 *****************************************************************************/
PRIVATE void tty_dev_read(TTY* tty)
{
	if (is_current_console(tty->p_console))
	{
	LOG_CALLS(p_proc_ready,"tty_dev_read");
		keyboard_read(tty);
	// LOG_RECORD("tty-buf:%s",tty->p_inbuf_head);
	LOG_RETS(p_proc_ready,"tty_dev_read");
	}
}


/*****************************************************************************
 				将p_tty的缓冲区数据写入进程P中
 *****************************************************************************/
PRIVATE void tty_dev_write(TTY* tty)
{
	// 处理tty缓冲区中的所有字符
	while (tty->inbuf_count) 
	{
		// if(tty->tty_procnr != NO_TASK)
		// 如果没有进程 想要读取tty数据,则溜溜溜...

		char ch = *(tty->p_inbuf_tail);
		tty->p_inbuf_tail = (tty->p_inbuf_tail - tty->in_buf + 1) % TTY_IN_BYTES + tty->in_buf;
		tty->inbuf_count--;

		// 如果进程P 还需要数据
		if (tty->tty_left_cnt && tty->tty_procnr != NO_TASK) 
		{
			if (ch >= ' ' && ch <= '~') //如果是可打印的字符
			{
				out_char(tty->p_console, ch); //显示字符

				// 将ch 送入进程P 的缓冲区中
				void * p = tty->tty_req_buf + tty->tty_trans_cnt;
				memcpy(p, (void *)va2la(TASK_TTY, &ch), 1);
			LOG_RECORD("\ntty send %c to %s's buf\n",ch,proc_table[tty->tty_procnr].name);
				tty->tty_trans_cnt++;
				tty->tty_left_cnt--;
			}
			else if (ch == '\b' && tty->tty_trans_cnt) 
			{
				out_char(tty->p_console, ch);

				// ch 不送入进程P 的缓冲区中
				// 
				tty->tty_trans_cnt--;
				tty->tty_left_cnt++;
			}

			// 一次"write"完成,一次的数据已经写入进程P 的缓冲中
			if (ch == '\n' || tty->tty_left_cnt == 0)
			{
				out_char(tty->p_console, '\n');

				// 数据传输完毕,恢复read进程P 
				MESSAGE msg;
				msg.type 	= RESUME_PROC;
				msg.PROC_NR = tty->tty_procnr;
				msg.CNT 	= tty->tty_trans_cnt;
				LOG_RECORD("\nOver:%s read data:%s\n", proc_table[tty->tty_procnr].name , (char *)(tty->tty_req_buf) );
				send_recv(SEND, tty->tty_caller, &msg);
				tty->tty_left_cnt = 0;
			}
		}
	}
}


/*****************************************************************************
 *                                tty_do_read
 *****************************************************************************/
/**
 * Invoked when task TTY receives DEV_READ message.
 *
 * @note The routine will return immediately after setting some members of
 * TTY struct, telling FS to suspend the proc who wants to read. The real
 * transfer (tty buffer -> proc buffer) is not done here.
 * 
 * @param tty  From which TTY the caller proc wants to read.
 * @param msg  The MESSAGE just received.
 *****************************************************************************/
PRIVATE void tty_do_read(TTY* tty, MESSAGE* msg)
{
	/* tell the tty: */
	tty->tty_caller   = msg->source;  	/* who called, usually FS */
	tty->tty_procnr   = msg->PROC_NR; 	/* who wants the chars */
	tty->tty_req_buf  = va2la(tty->tty_procnr, msg->BUF);	/* where the chars should be put */
	tty->tty_left_cnt = msg->CNT; 		/* how many chars are requested */
	tty->tty_trans_cnt= 0; 				/* how many chars have been transferred */

	msg->type 	= SUSPEND_PROC;
	msg->CNT 	= tty->tty_left_cnt;
	send_recv(SEND, tty->tty_caller, msg);	//让task_fs去挂起需要数据的进程P
}


/*****************************************************************************
 *                                tty_do_write
 *****************************************************************************/
/**
 * Invoked when task TTY receives DEV_WRITE message.
 * 
 * @param tty  To which TTY the calller proc is bound.
 * @param msg  The MESSAGE.
 *****************************************************************************/
PRIVATE void tty_do_write(TTY* tty, MESSAGE* msg)
{
	char 		buf[TTY_OUT_BUF_LEN];
	// 为什么要用缓冲区域? 不可以直接读写吗?

	char * p 	= (char*)va2la(msg->PROC_NR, msg->BUF);
	int  count	= msg->CNT;

	while (count) 
	{
		int bytes = min(TTY_OUT_BUF_LEN, count);

		// 讲进程P 要'写'的数据复制到buf 缓冲中先
		memcpy(va2la(TASK_TTY, buf),
			   (void*)p, 
			    bytes);
		
		// 再将buf 缓冲中的数据输出
		for (int j = 0; j < bytes; j++)
		{
			out_char(tty->p_console, buf[j]);
		}
		count -= bytes;
		p += bytes;
	}

	msg->type = SYSCALL_RET;
	send_recv(SEND, msg->source, msg);
}


/*****************************************************************************
 *                                sys_printx
 *****************************************************************************/
/**
 * System calls accept four parameters. `printx' needs only two, so it wastes
 * the other two.
 *
 * @note `printx' accepts only one parameter -- `char* s', the other one --
 * `struct proc * proc' -- is pushed by kernel.asm::sys_call so that the
 * kernel can easily know who invoked the system call.
 *
 * @note s[0] (the first char of param s) is a magic char. if it equals
 * MAG_CH_PANIC, then this syscall was invoked by `panic()', which means
 * something goes really wrong and the system is to be halted; if it equals
 * MAG_CH_ASSERT, then this syscall was invoked by `assert()', which means
 * an assertion failure has occured. @see kernel/main lib/misc.c.
 * 
 * @param _unused1  Ignored.
 * @param _unused2  Ignored.
 * @param s         The string to be printed.
 * @param p_proc    Caller proc.
 * 
 * @return  Zero if success.
 *****************************************************************************/
PUBLIC int sys_printx(int _unused1, int _unused2, char* s, PROCESS* p_proc)
{
	const char * p;
	char ch;

	char reenter_err[] = "? k_reenter is incorrect for unknown reason";
	reenter_err[0] = MAG_CH_PANIC;

	/**
	 * @note Code in both Ring 0 and Ring 1~3 may invoke printx().
	 * If this happens in Ring 0, no linear-physical address mapping
	 * is needed.
	 *
	 * @attention The value of `k_reenter' is tricky here. When
	 *   -# printx() is called in Ring 0
	 *      - k_reenter > 0. When code in Ring 0 calls printx(),
	 *        an `interrupt re-enter' will occur (printx() generates
	 *        a software interrupt). Thus `k_reenter' will be increased
	 *        by `kernel.asm::save' and be greater than 0.
	 *   -# printx() is called in Ring 1~3
	 *      - k_reenter == 0.
	 */
	if (k_reenter == 0)  /* printx() called in Ring<1~3> */
		p = va2la(proc2pid(p_proc), s);
	else if (k_reenter > 0) /* printx() called in Ring<0> */
		p = s;
	else	/* this should NOT happen */
		p = reenter_err;

	/**
	 * @note if assertion fails in any TASK, the system will be halted;
	 * if it fails in a USER PROC, it'll return like any normal syscall
	 * does.
	 */
	if ((*p == MAG_CH_PANIC) ||
	    (*p == MAG_CH_ASSERT && p_proc_ready < &proc_table[NR_TASKS])) 
	{
		disable_int();
		char * v = (char*)V_MEM_BASE;
		const char * q = p + 1; /* +1: skip the magic char */

		while (v < (char*)(V_MEM_BASE + V_MEM_SIZE)) {
			*v++ = *q++;
			*v++ = RED_CHAR;
			if (!*q) {
				while (((int)v - V_MEM_BASE) % (SCREEN_WIDTH * 16)) {
					/* *v++ = ' '; */
					v++;
					*v++ = GRAY_CHAR;
				}
				q = p + 1;
			}
		}

		__asm__ __volatile__("ud2");
	}

	while ((ch = *p++) != 0) 
	{
		if (ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT)
			continue; /* skip the magic char */

		/* TTY * ptty; */
		/* for (ptty = TTY_FIRST; ptty < TTY_END; ptty++) */
		/* 	out_char(ptty->console, ch); /\* output chars to all TTYs *\/ */
		out_char(TTY_FIRST->p_console, ch);
	}

	return 0;
}

