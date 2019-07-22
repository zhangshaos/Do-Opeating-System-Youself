/*================================================================================================
File name:	lib/printf.c
Description:	*标准c的printf函数
Copyright:	Chauncey Zhang
Date:		2019-7-21
===============================================================================================*/

#include"type.h"
#include"func_proto.h"


/*======================================================================*
                                vsprintf
        =========================================================
                            格式化(由printf()调用)
 *======================================================================*/
int vsprintf(char *buf, const char *fmt, va_list args)
{
    char 	*p = buf;
	char	tmp[16];
	va_list	p_next_arg = args;

	for ( /* char *p = buf */ ; '\0' != *fmt; fmt++) 
	{
		if (*fmt != '%')
		{
            /* 如果字符不是'%',则将其从fmt字符串复制到buf中 */
		    *p++ = *fmt;
			continue;
		}

        /* 此时fmt指向'%'后面的字符 */
		fmt++;

		switch (*fmt)
		{
		case 'x':
            /* 将整数型参数转化为16禁止字符串显示, 并copy到buf尾部 */
		    itoa(tmp, *((int*)p_next_arg));
			strcpy(p, tmp);
			p += strlen(tmp);

			p_next_arg += 4;
			break;
		case 's':

			break;
		default:
			break;
		}
	}

	return (p - buf);
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
	write(buf, i);

	return i;
}

