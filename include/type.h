/*================================================================================================
File name:		include/type.h
Description:	*typedef
Copyright:		Chauncey Zhang
Date:		 	2019-7-15
===============================================================================================*/


#ifndef __TYPE_H__
#define __TYPE_H__


typedef	unsigned int	u32;
typedef	unsigned short	u16;
typedef	unsigned char	u8;

typedef	char *			va_list;

typedef	void	(*int_handler)();
typedef	void	(*task_f)();
typedef	void	(*irq_handler)(int irq);
typedef void*	system_call;


#endif