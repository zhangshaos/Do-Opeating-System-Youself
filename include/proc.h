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
	char p_name[16];           /* name of the process */

	int nr_tty;					/* just for simplifying, every Process have its TTY. */
}PROCESS;

/* Task描述结构 */
typedef struct s_task {
	task_f	initial_eip;
	int	stacksize;
	char	name[32];
}TASK;


/* Number of tasks & procs */
#define NR_TASKS	1
#define NR_PROCS	3	/* user process */

/* stacks of tasks */
#define STACK_SIZE_TTY		0x8000
#define STACK_SIZE_TESTA	0x8000
#define STACK_SIZE_TESTB	0x8000
#define STACK_SIZE_TESTC	0x8000

#define STACK_SIZE_TOTAL	(STACK_SIZE_TTY + \
				STACK_SIZE_TESTA + \
				STACK_SIZE_TESTB + \
				STACK_SIZE_TESTC)

#endif