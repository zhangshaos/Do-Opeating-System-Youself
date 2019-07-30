/*================================================================================================
File name:		include/proc.h
Description:	*定义描述进程的结构
Copyright:		Chauncey Zhang
Date:		 	2019-7-13
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

#ifndef __PROC_H__
#define __PROC_H__

#include "proc.h"
#include "protect.h"
#include "type.h"

typedef struct s_stackframe {	/* proc_ptr points here				↑ Low			*/
	u32	gs;			/* ┓						│			*/
	u32	fs;			/* ┃						│			*/
	u32	es;			/* ┃						│			*/
	u32	ds;			/* ┃						│			*/
	u32	edi;		/* ┃						│			*/
	u32	esi;		/* ┣ pushed by save()				│			*/
	u32	ebp;		/* ┃						│			*/
	u32	kernel_esp;	/* <- 'popad' will ignore it			│			*/
	u32	ebx;		/* ┃						↑栈从高地址往低地址增长*/		
	u32	edx;		/* ┃						│			*/
	u32	ecx;		/* ┃						│			*/
	u32	eax;		/* ┛						│			*/
	u32	retaddr;	/* return address for assembly code save()	│			*//* 产生跨特权级中断时, ss,esp,eflags,cs,eip自动压栈. 然后哦call save填充此. */
	u32	eip;		/*  ┓						│			*/
	u32	cs;			/*  ┃						│			*/
	u32	eflags;		/*  ┣ these are pushed by CPU during interrupt	│			*/
	u32	esp;		/*  ┃						│			*/
	u32	ss;			/*  ┛						┷High			*/
}STACK_FRAME;

/* 类似进程控制块PCB */
typedef struct s_proc {
	STACK_FRAME regs;          /* process registers saved in stack frame */

	u16 ldt_sel;               /* gdt selector giving ldt base and limit */
	DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data */

        int ticks;                 /* remained ticks */
        int priority;

	u32 pid;                   /* process id passed in from MM */
	char name[16];				/* name of the process */

	/* p_status.(ready, sending, receiving) */
	int  p_flags;              /* process flags. A proc is runnable iff p_flags==0*/

	/**
	 * 本进程持有的消息.
	 * SENDING状态:	表示此进程正在发送的消息(还没有目标进程接受,所以此进程阻塞)
	 * RECEIVING状态:表示此进程想要接受的消息(还没有进程发送,所以此进程阻塞)
	 * REARY状态:	p_hold_msg = 0,因为此进程没有消息要处理 
	 */
	MESSAGE * p_hold_msg;

	/* p_want_recvform */
	int p_recvfrom;				/* 'p' means index of proc_table[] */
	/* p_want_sendto */
	int p_sendto;

	int has_int_msg;           /* 如果非0, 表示该进程有一个待处理的硬件中断 */

	struct s_proc * q_sending;  		/* pointer to the first process that deliver msg to this process */

	struct s_proc * next_sending;		/* the next process that deliver msg to the destination of this process  */

	int nr_tty;					/* just for simplifying, every Process have its TTY. */
}PROCESS;

/* Task描述结构 */
typedef struct s_task {
	task_f	initial_eip;
	int	stacksize;
	char	name[32];
}TASK;

#define proc2pid(x) (x - proc_table)

/* Number of tasks & procs */
#define NR_TASKS	4
#define NR_PROCS	3
#define FIRST_PROC	proc_table[0]
#define LAST_PROC	proc_table[NR_TASKS + NR_PROCS - 1]

/* stacks of tasks */
#define STACK_SIZE_TTY		0x8000
#define STACK_SIZE_SYS		0x8000
#define STACK_SIZE_HD		0x8000
#define STACK_SIZE_FS		0x8000
#define STACK_SIZE_TESTA	0x8000
#define STACK_SIZE_TESTB	0x8000
#define STACK_SIZE_TESTC	0x8000

#define STACK_SIZE_TOTAL	(STACK_SIZE_TTY + \
				STACK_SIZE_SYS + \
				STACK_SIZE_HD + \
				STACK_SIZE_FS + \
				STACK_SIZE_TESTA + \
				STACK_SIZE_TESTB + \
				STACK_SIZE_TESTC)

#endif
