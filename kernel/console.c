/*================================================================================================
File name:		kernel/console.c
Description:	*
Copyright:		Chauncey Zhang
Date:		 	2019-7-18
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


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

PRIVATE void 	set_cursor(unsigned int position);
PRIVATE void 	set_video_start_addr(u32 addr);
PRIVATE void 	flush(CONSOLE* p_con);
PRIVATE	void 	w_copy(unsigned int dst, const unsigned int src, int size);
PRIVATE void	clear_screen(int pos, int len);

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE / 2;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;
	p_tty->p_console->is_full			 = 0;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		/* 
		 * `?' in this string will be replaced with 0, 1, 2, ...
		 */
		const char prompt[] = "[TTY #?]\n";
		const char * p = prompt;
		for (; *p; p++)
		{
			out_char(p_tty->p_console, *p == '?' ? nr_tty + '0' : *p);
		}
	}

	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == console_table + nr_current_console);
}


/*======================================================================*
			   					out_char
	==================================================================
					将字符的ascii码和颜色属性写到显存中 
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);

	assert(p_con->cursor - p_con->original_addr < p_con->v_mem_limit);

	/*
	 * calculate the coordinate of cursor in current console (not in
	 * current screen)
	 */
	int cursor_x = ( p_con->cursor - p_con->original_addr ) % SCREEN_WIDTH;
	int cursor_y = ( p_con->cursor - p_con->original_addr ) / SCREEN_WIDTH;

	switch(ch) 
	{
	case '\n':
		p_con->cursor = p_con->original_addr + SCREEN_WIDTH * (cursor_y + 1);
		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr)
		{
			p_con->cursor--;
			*(p_vmem-2) = ' ';
			*(p_vmem-1) = DEFAULT_CHAR_COLOR;
		}
		break;
	default:
		*p_vmem++ = ch;
		*p_vmem++ = DEFAULT_CHAR_COLOR;
		p_con->cursor++;
		break;
	}

	if (p_con->cursor - p_con->original_addr >= p_con->v_mem_limit) 
	{
		cursor_x = ( p_con->cursor - p_con->original_addr ) % SCREEN_WIDTH;
	 	cursor_y = ( p_con->cursor - p_con->original_addr ) / SCREEN_WIDTH;

		// 这儿是在干什么?
		int cp_orig = p_con->original_addr + (cursor_y + 1) * SCREEN_WIDTH - SCREEN_SIZE;
		w_copy(p_con->original_addr, cp_orig, SCREEN_SIZE - SCREEN_WIDTH);

		p_con->current_start_addr 	= p_con->original_addr;
		p_con->cursor	 			= p_con->original_addr + (SCREEN_SIZE - SCREEN_WIDTH) + cursor_x;

		clear_screen(p_con->cursor, SCREEN_WIDTH);
		p_con->is_full 				= 1;
	}

	assert(p_con->cursor - p_con->original_addr < p_con->v_mem_limit);

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE ||
	       p_con->cursor < p_con->current_start_addr) 
	{
		scroll_screen(p_con, SCR_UP);

		clear_screen(p_con->cursor, SCREEN_WIDTH);
	}

	flush(p_con);
}


/*****************************************************************************
 *                                clear_screen
 *****************************************************************************/
/**
 * Write whitespaces to the screen.
 * 
 * @param pos  Write from here.
 * @param len  How many whitespaces will be written.
 *****************************************************************************/
PRIVATE void clear_screen(int pos, int len)
{
	u8 * pch = (u8*)(V_MEM_BASE + pos * 2);
	while (--len >= 0) 
	{
		*pch++ = ' ';
		*pch++ = DEFAULT_CHAR_COLOR;
	}
}


/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
	if (is_current_console(p_con)) 
	{
		set_cursor(p_con->cursor);
		set_video_start_addr(p_con->current_start_addr);
	}
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	flush(&console_table[nr_console]);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* con, int dir)
{
	// fuck, 这三个变量名字起得不明所以!!!
	// 语文是白学了.
	int oldest; 	/* addr of the oldest available line in the console */
	int newest; 	/* .... .. ... latest ......... .... .. ... ....... */
	int scr_top;	/* position of the top of current screen */

	newest 		= (con->cursor - con->original_addr);
	oldest 		= con->is_full ? (newest + SCREEN_WIDTH) % con->v_mem_limit : 0;
	scr_top 	= con->current_start_addr - con->original_addr;

	if (dir == SCR_DN) 
	{
		if (!con->is_full && scr_top > 0) 
		{
			con->current_start_addr -= SCREEN_WIDTH;
		}
		else if (con->is_full && scr_top != oldest) 
		{
			if (con->cursor - con->original_addr >= con->v_mem_limit - SCREEN_SIZE) 
			{
				if (con->current_start_addr != con->original_addr)
					con->current_start_addr -= SCREEN_WIDTH;
			}
			else if (con->current_start_addr == con->original_addr) 
			{
				scr_top = con->v_mem_limit - SCREEN_SIZE;
				con->current_start_addr = con->original_addr + scr_top;
			}
			else 
			{
				con->current_start_addr -= SCREEN_WIDTH;
			}
		}
	}
	else if (dir == SCR_UP) 
	{
		if (!con->is_full && newest >= scr_top + SCREEN_SIZE) 
		{
			con->current_start_addr += SCREEN_WIDTH;
		}
		else if (con->is_full && scr_top + SCREEN_SIZE - SCREEN_WIDTH != newest) 
		{
			if (scr_top + SCREEN_SIZE == con->v_mem_limit)
				con->current_start_addr = con->original_addr;
			else
				con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else 
	{
		assert(dir == SCR_DN || dir == SCR_UP);
	}

	flush(con);
}


/*****************************************************************************
 *                                w_copy
 *****************************************************************************/
/**
 * Copy data in WORDS.
 *
 * Note that the addresses of dst and src are not pointers, but integers, 'coz
 * in most cases we pass integers into it as parameters.
 * 
 * @param dst   Addr of destination.
 * @param src   Addr of source.
 * @param size  How many words will be copied.
 *****************************************************************************/
PRIVATE	void w_copy(unsigned int dst, const unsigned int src, int size)
{
	memcpy((void*)(V_MEM_BASE + (dst << 1)),
		  (void*)(V_MEM_BASE + (src << 1)),
		  size << 1);
}