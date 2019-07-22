/*================================================================================================
File name:		kernel/console.c
Description:	*控制台(VGA屏幕)
Copyright:		Chauncey Zhang
Date:		 	2019-7-21
===============================================================================================*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include"const.h"
#include"type.h"
#include"struct_console.h"
#include"struct_tty.h"
#include"global.h"
#include"func_proto.h"


PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);

/*======================================================================*
			   				init_screen
	================================================================
							初始化屏幕(控制台)
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	const int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	const int v_mem_size = V_MEM_SIZE / 2;	/* 显存总大小 (in WORD) */

	const int con_v_mem_size			= v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr     = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit       = con_v_mem_size;
	p_tty->p_console->current_start_addr= p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;	/* 屏幕上的每个字符对应两个字节,除以2得到以屏幕字符(属性+ascii)为单位的位置 */
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, '0' + nr_tty);
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}



/*======================================================================*
			   					out_char
	==================================================================
					将字符的ascii码和颜色属性写到显存中 
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);

	switch(ch) {
	case '\n':
		if (p_con->cursor < p_con->original_addr
							+ p_con->v_mem_limit
							- SCREEN_WIDTH)
		{
			p_con->cursor = p_con->original_addr
								+ SCREEN_WIDTH * ((p_con->cursor - p_con->original_addr) / SCREEN_WIDTH + 1);
		}
		break;
	case '\b':
		if (p_con->cursor > p_con->original_addr)
		{
			p_con->cursor--;
			/* 低字节ascii码, 高字节属性 */
			*(p_vmem-2) = ' ';
			*(p_vmem-1) = DEFAULT_CHAR_COLOR;
		}
		break;
	default:
		if (p_con->cursor < p_con->original_addr 
								+ p_con->v_mem_limit - 1)
		{
			*p_vmem++ = ch;
			*p_vmem++ = DEFAULT_CHAR_COLOR;
			p_con->cursor++;
		}
		break;
	}

	/* 如果字符超过屏幕了, 则卷轴 */
	if(p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE)
	{
		scroll_screen(p_con, SCR_DN);
	}
	else if(p_con->cursor < p_con->current_start_addr)
	{
		scroll_screen(p_con, SCR_UP);
	}

	flush(p_con);
}


/*======================================================================*
                           flush
	===============================================================
					1.刷新光标位置
					2.刷新VGA屏幕显示的显存起始位置
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
	=================================================================
							设置光标位置
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
	=================================================================
						设置VGA屏幕显示的显存起始位置
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
			   				scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr)
		{
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE
			< p_con->original_addr + p_con->v_mem_limit)
		{
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
		/* do nothing... */
	}

	flush(p_con);
}


/*======================================================================*
			   			is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == console_table + nr_current_console);
}


/*======================================================================*
			   				select_console
	================================================================
					超过可选择的console范围则do nothing
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	flush(&console_table[nr_console]);
}