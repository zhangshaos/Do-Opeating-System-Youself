/*================================================================================================
File name:		lib/printf.c
Description:	*标准c的printf函数
Copyright:		Chauncey Zhang
Date:			2019-7-21
===============================================================================================*/

#include"type.h"
#include"func_proto.h"
#include "global.h"
#include "const.h"



/*======================================================================*
                                i2a
 *======================================================================*/
PRIVATE char* i2a(int val, int base, char ** ps)
{
	int m = val % base;
	int q = val / base;
	if (q) {
		i2a(q, base, ps);
	}
	*(*ps)++ = (m < 10) ? (m + '0') : (m - 10 + 'A');

 	return *ps;
}


/*======================================================================*
                              sys_printx
*======================================================================*/
PUBLIC int sys_printx(int __unused_param1__, int __unused_param2__,
					  char* s, PROCESS* p_proc)
{
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
	char * p;
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

 		while (v < (char*)(V_MEM_BASE + V_MEM_SIZE))
		{
			*v++ = *q++;
			*v++ = RED_CHAR;
			if (!*q) 
			{
				while (((int)v - V_MEM_BASE) % (SCREEN_WIDTH * 16)) 
				{
					/* *v++ = ' '; */
					v++;
					*v++ = GRAY_CHAR;
				}
				q = p + 1;
			}
		}

 		__asm__ __volatile__("hlt");
	}

	char ch;
 	while ((ch = *p++) != 0) 
	{
		if (ch == MAG_CH_PANIC || ch == MAG_CH_ASSERT)
			continue; /* skip the magic char */

 		out_char(tty_table[p_proc->nr_tty].p_console, ch);
	}

 	return 0;
}


/*======================================================================*
                                vsprintf
        =========================================================
                            格式化(由printf()调用)
 *======================================================================*/
PUBLIC int vsprintf(char *buf, const char *fmt, va_list args)
{
	char*	p;	
 	va_list	p_next_arg = args;
	int		m;

 	char	inner_buf[STR_DEFAULT_LEN];
	char	cs;
	int		align_nr;


 	for (p=buf;*fmt;fmt++)
	{
		if (*fmt != '%')	
		{	
			*p++ = *fmt;
			continue;
		}
		else {		/* a format string begins */
			align_nr = 0;
		}


 		fmt++;


 		if (*fmt == '%') 
		{
			*p++ = *fmt;
			continue;
		}
		else if (*fmt == '0') 
		{
			cs = '0';
			fmt++;
		}
		else 
		{
			cs = ' ';
		}
		while (((unsigned char)(*fmt) >= '0') 
				&& ((unsigned char)(*fmt) <= '9')) 
		{
			align_nr *= 10;
			align_nr += *fmt - '0';
			fmt++;
		}

 		char * q = inner_buf;
		memset(q, 0, sizeof(inner_buf));

 		switch (*fmt) 
		{
		case 'c':
			*q++ = *((char*)p_next_arg);
			p_next_arg += 4;
			break;
		case 'x':
			m = *((int*)p_next_arg);
			i2a(m, 16, &q);
			p_next_arg += 4;
			break;
		case 'd':
			m = *((int*)p_next_arg);
			if (m < 0)
			{
				m = m * (-1);
				*q++ = '-';
			}
			i2a(m, 10, &q);
			p_next_arg += 4;
			break;
		case 's':
			strcpy(q, (*((char**)p_next_arg)));
			q += strlen(*((char**)p_next_arg));
			p_next_arg += 4;
			break;
		default:
			break;
		}


		for (int k = 0; 
			 k < ((align_nr > strlen(inner_buf)) ? (align_nr - strlen(inner_buf)) : 0);
			 k++) 
		{
			*p++ = cs;
		}

		q = inner_buf;
		while (*q) 
		{
			*p++ = *q++;
		}
	}


 	*p = 0;

 	return (p - buf);
}



 /*======================================================================*
                                 sprintf
 *======================================================================*/
int sprintf(char *buf, const char *fmt, ...)
{
	va_list arg = (va_list)((char*)(&fmt) + 4);        /* 4 是参数 fmt 所占堆栈中的大小 */
	return vsprintf(buf, fmt, arg);
}


/*======================================================================*
                                 printf
 *======================================================================*/
int printf(const char *fmt, ...)
{
	int i;
	char buf[256];  /* 存储格式化转换后的字符串 */
	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/

	i = vsprintf(buf, fmt, arg);
	buf[i] = 0;
	printx(buf);

	return i;
}

