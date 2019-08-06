/*================================================================================================
File name:		kernel/tty.c
Description:	*tty任务(包括读和写)
Copyright:		Chauncey Zhang
Date:		 	2019-7-18
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/


#include"const.h"
#include"type.h"
#include"struct_tty.h"
#include"struct_keyboard.h"
#include"global.h"
#include"func_proto.h"


#define TTY_FIRST	(tty_table)
#define TTY_END		(tty_table + NR_CONSOLES)

PRIVATE void init_tty(TTY* p_tty);
PRIVATE void tty_do_read(TTY* p_tty);
PRIVATE void tty_do_write(TTY* p_tty);
PRIVATE void put_key(TTY* p_tty, char key);


/*======================================================================*
                           task_tty
	================================================================
							TTY任务
 *======================================================================*/
PUBLIC void task_tty()
{
	TTY*	p_tty;

	/* 允许键盘中断 */
	init_keyboard();

	/* 初始化所有tty */
	for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) 
	{
		init_tty(p_tty);
	}

	/* 默认选择0号控制台 */
	select_console(0);

	while (1) 
	{
		for (p_tty=TTY_FIRST;p_tty<TTY_END;p_tty++) 
		{
			tty_do_read(p_tty);
			tty_do_write(p_tty);
		}
	}
}


/*======================================================================*
			   				init_tty
	=================================================================
						初始化控制台屏幕
 *======================================================================*/
PRIVATE void init_tty(TTY* p_tty)
{
	p_tty->inbuf_count = 0;
	p_tty->p_inbuf_head = p_tty->p_inbuf_tail = p_tty->in_buf;

	/* 初始化屏幕(控制台) */
	init_screen(p_tty);
}


/*======================================================================*
			     			tty_do_read
 *======================================================================*/
PRIVATE void tty_do_read(TTY* p_tty)
{
	/* 只有当前的控制台才可以读取键盘数据 */
	if (is_current_console(p_tty->p_console)) 
	{
		keyboard_read(p_tty);
	}
}


/*======================================================================*
			      			tty_do_write
 *======================================================================*/
PRIVATE void tty_do_write(TTY* p_tty)
{
	/* 如果该tty有等待输出的字符 */
	if (p_tty->inbuf_count) 
	{
		char ch = *(p_tty->p_inbuf_tail);

		/* p_inbuf_tail 指向下一个要输出的字符 */
		p_tty->p_inbuf_tail = p_tty->in_buf + (p_tty->p_inbuf_tail - p_tty->in_buf + 1)%TTY_IN_BYTES;

		p_tty->inbuf_count--;

		out_char(p_tty->p_console, ch);
	}
}


/*======================================================================*
			      			put_key
	================================================================
				将key放置到tty.in_buf[]中, 等待输出到屏幕
*======================================================================*/
PRIVATE void put_key(TTY* p_tty, char key)
{
	if (p_tty->inbuf_count < TTY_IN_BYTES) 
	{
		*(p_tty->p_inbuf_head) = key;

		/* p_inbuf_head 指向下一个空闲的缓冲区 */
		p_tty->p_inbuf_head = p_tty->in_buf + (p_tty->p_inbuf_head - p_tty->in_buf + 1)%TTY_IN_BYTES;
		
		p_tty->inbuf_count++;
	}
}


/*======================================================================*
							respond_key
	=================================================================
				得到按键key后, 响应按键(打印字符,还是执行功能)
 *======================================================================*/
PUBLIC void respond_key(TTY* p_tty, u32 key)
{
        if (!(key & FLAG_EXT))	/* 如果是可打印字符, 则将其打印出来 */
		{
			put_key(p_tty, (char)key);
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
					scroll_screen(p_tty->p_console, SCR_UP);        
				}
				break;
			case DOWN:
				if ((key & FLAG_SHIFT_L) || (key & FLAG_SHIFT_R))
				{
					scroll_screen(p_tty->p_console, SCR_DN);
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
                              tty_write
	===============================================================
						系统调用中断处理程序调用
*======================================================================*/
PUBLIC void tty_write(TTY* p_tty, char* buf, int len)
{
        char* p = buf;
        int i = len;

        while (i) 
		{
            out_char(p_tty->p_console, *p++);
            i--;
        }
}

