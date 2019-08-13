/*================================================================================================
File name:	kernel/console.c
Description:	*
Copyright:	Chauncey Zhang
Date:		2019-7-18
Other:		参见<Orange's 一个操作系统的实现>
===============================================================================================*/

#include "type.h"
#include "const.h"
#include "proto.h"
#include "stdio.h"
#include "log.h"


/*======================================================================*
                                <Ring1-3> printf
 *======================================================================*/
int printf(const char *fmt, ...)
{
	int i;
	char buf[STR_DEFAULT_LEN];

	va_list arg = (va_list)((char*)(&fmt) + 4); /*4是参数fmt所占堆栈中的大小*/
	i = vsprintf(buf, fmt, arg);
	// LOG_CALLS(p_proc_ready,"before write");
	int c = write(1, buf, i);
	// LOG_RETS(p_proc_ready,"after write");
	assert(c == i);

	return i;
}

/*****************************************************************************
 *                                printl
 *****************************************************************************/
/**
 * low level print
 * 
 * @param fmt  The format string
 * 
 * @return  The number of chars printed.
 *****************************************************************************/
PUBLIC int printl(const char *fmt, ...)
{
	int i;
	char buf[STR_DEFAULT_LEN];

	va_list arg = (va_list)((char*)(&fmt) + 4); /**
						     * 4: size of `fmt' in
						     *    the stack
						     */
	i = vsprintf(buf, fmt, arg);
	printx(buf);

	return i;
}

