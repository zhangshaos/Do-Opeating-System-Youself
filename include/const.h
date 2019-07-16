/*================================================================================================
File name:		include/const.h
Description:	*定义了基本常量
Copyright:		Chauncey Zhang
Date:		 	2019-6-29
===============================================================================================*/

#ifndef	__CONST_H__
#define	__CONST_H__


/* EXTERN is defined as extern except in global.c */
#define EXTERN extern

/* 函数类型 */
#define	PUBLIC		    /* PUBLIC is the opposite of PRIVATE */
#define	PRIVATE	static	/* PRIVATE x limits the scope of x */

/* Boolean */
#define	TRUE	1
#define	FALSE	0

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128
#define	IDT_SIZE	256

/* 每个 LDT 中描述符的个数 */
#define LDT_SIZE    2   /* 一个数据段,一个代码段描述符 */

/* 权限 */
#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3

/* RPL */
/* there are three kinds of processing(Task, Serve, User)*/
#define	RPL_KRNL    0
#define	RPL_TASK	1
#define	RPL_USER	3


#endif /* _ORANGES_CONST_H_ */
