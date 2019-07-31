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

/* 字符串最大长度 */
#define	STR_DEFAULT_LEN	1024

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128
#define	IDT_SIZE	256

/* 每个 LDT 中描述符的个数 */
#define LDT_SIZE    2   /* 一个数据段,一个代码段描述符 */
/* descriptor indices in LDT */
#define INDEX_LDT_C     0
#define INDEX_LDT_RW    1

/* 权限 */
#define	PRIVILEGE_KRNL	0
#define	PRIVILEGE_TASK	1
#define	PRIVILEGE_USER	3

/* RPL */
/* there are three kinds of processing(Task, Serve, User)*/
#define	RPL_KRNL    0
#define	RPL_TASK	1
#define	RPL_USER	3

/* TTY */
#define NR_CONSOLES	3	/* consoles */

 /* IPC */
#define SEND		1
#define RECEIVE		2
#define BOTH		3	/* BOTH = (SEND | RECEIVE) */

/* magic chars used by `printx' */
#define MAG_CH_PANIC	'\002'
#define MAG_CH_ASSERT	'\003'

 /**
 * @enum msgtype
 * @brief MESSAGE types
 */
enum msgtype {
	/* 
	 * when hard interrupt occurs, a msg (with type==HARD_INT) will
	 * be sent to some tasks
	 */
	HARD_INT = 1,

 	/* SYS task */
	GET_TICKS,

	/* message type for drivers */
	DEV_OPEN = 1001,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL
};


/* macros for messages */
/* #define	FD			u.m3.m3i1 */
/* #define	PATHNAME	u.m3.m3p1 */
/* #define	FLAGS		u.m3.m3i1 */
/* #define	NAME_LEN	u.m3.m3i2 */
#define	CNT			u.m3.m3i2	/* 磁盘读写时的字节数 */
#define	REQUEST		u.m3.m3i2
#define	PROC_NR		u.m3.m3i3
#define	DEVICE		u.m3.m3i4
#define	POSITION	u.m3.m3l1
#define	BUF			u.m3.m3p2
/* #define	OFFSET		u.m3.m3i2 */
/* #define	WHENCE		u.m3.m3i3 */

 /* #define	PID			u.m3.m3i2 */
/* #define	STATUS		u.m3.m3i1 */
#define	RETVAL		u.m3.m3i1
/* #define	STATUS		u.m3.m3i1 */





#endif /* _ORANGES_CONST_H_ */
