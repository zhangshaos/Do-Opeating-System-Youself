/*================================================================================================
File name:		include/type.h
Description:	*定义某种类型的别名
Copyright:		Chauncey Zhang
Date:		 	2019-6-29
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

#ifndef	_ORANGES_TYPE_H_
#define	_ORANGES_TYPE_H_


typedef	unsigned int		u32;
typedef	unsigned short		u16;
typedef	unsigned char		u8;

typedef	char *			va_list;

typedef	void	(*int_handler)	();
typedef	void	(*task_f)	();
typedef	void	(*irq_handler)	(int irq);

typedef void*	system_call;


#endif /* _ORANGES_TYPE_H_ */
