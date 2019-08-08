/*================================================================================================
File name:		include/type.h
Description:	*typedef
Copyright:		Chauncey Zhang
Date:		 	2019-7-15
===============================================================================================*/


#ifndef __TYPE_H__
#define __TYPE_H__

typedef unsigned long long  u64;
typedef	unsigned int	    u32;
typedef	unsigned short	    u16;
typedef	unsigned char	    u8;

typedef	char *			va_list;

typedef	void	(*int_handler)();
typedef	void	(*task_f)();
typedef	void	(*irq_handler)(int irq);
typedef void*	system_call;


/**
 * MESSAGE mechanism is borrowed from MINIX
 */
struct mess1 {
	int m1i1;
	int m1i2;
	int m1i3;
	int m1i4;
};
struct mess2 {
	void* m2p1;
	void* m2p2;
	void* m2p3;
	void* m2p4;
};
struct mess3 {
	int	m3i1;
	int	m3i2;
	int	m3i3;
	int	m3i4;
	u64	m3l1;
	u64	m3l2;
	void*	m3p1;
	void*	m3p2;
};

typedef struct {
	int source;
	int type;
	union {
		struct mess1 m1;
		struct mess2 m2;
		struct mess3 m3;
	} u;
} MESSAGE;





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
	GET_TICKS,GET_PID,

	/* FS */
	OPEN, CLOSE, READ, WRITE, LSEEK, STAT, UNLINK,

	/* TTY, SYS, FS, MM, etc */
	SYSCALL_RET,

	/* message type for drivers */
	DEV_OPEN = 1001,
	DEV_CLOSE,
	DEV_READ,
	DEV_WRITE,
	DEV_IOCTL
};




/* macros for messages */
#define	FD			u.m3.m3i1
#define	PATHNAME	u.m3.m3p1
#define	FLAGS		u.m3.m3i1
#define	NAME_LEN	u.m3.m3i2
#define	CNT			u.m3.m3i2	/* 磁盘读写时的字节数 */
#define	REQUEST		u.m3.m3i2
#define	PROC_NR		u.m3.m3i3
#define	DEVICE		u.m3.m3i4
#define	POSITION	u.m3.m3l1
#define	BUF			u.m3.m3p2
#define	OFFSET		u.m3.m3i2 
#define	WHENCE		u.m3.m3i3 

#define	PID			u.m3.m3i2
/* #define	STATUS		u.m3.m3i1 */
#define	RETVAL		u.m3.m3i1






#endif