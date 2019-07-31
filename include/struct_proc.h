/*================================================================================================
File name:		include/struct_proc.h
Description:	*定义描述进程的结构
Copyright:		Chauncey Zhang
Date:		 	2019-7-13
===============================================================================================*/



#ifndef __PROCESS_H__
#define __PROCESS_H__

#include"type.h"
#include"const.h"
#include"struct_descript.h"

/* 任务状态段 */
typedef struct s_tss {
	u32	backlink;
	u32	esp0;		/* stack pointer to use during interrupt */
	u32	ss0;		/*   "   segment  "  "    "        "     */
	u32	esp1;
	u32	ss1;
	u32	esp2;
	u32	ss2;
	u32	cr3;
	u32	eip;
	u32	flags;
	u32	eax;
	u32	ecx;
	u32	edx;
	u32	ebx;
	u32	esp;
	u32	ebp;
	u32	esi;
	u32	edi;
	u32	es;
	u32	cs;
	u32	ss;
	u32	ds;
	u32	fs;
	u32	gs;
	u32	ldt;
	u16	trap;
	u16	iobase;	/* I/O位图基址大于或等于TSS段界限，就表示没有I/O许可位图 */
	/*u8	iomap[2];*/
}TSS;


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
	u32	retaddr;	/* return address for assembly code save()	│			*/
	u32	eip;		/*  ┓						│			*/
	u32	cs;			/*  ┃						│			*/
	u32	eflags;		/*  ┣ these are pushed by CPU during interrupt	│			*/
	u32	esp;		/*  ┃						│			*/
	u32	ss;			/*  ┛						┷High			*/
}STACK_FRAME;

/* 进程 */
typedef struct Proc {
	STACK_FRAME regs;	/* process registers saved in stack frame */

	u16 ldt_sel;	/* gdt selector giving ldt base and limit */
	
	/* LDT */
	DESCRIPTOR ldts[LDT_SIZE]; 

    int ticks;		/* remained ticks */
    int priority;

	u32 pid;		/* process id passed in from MM */
	char name[16];	/* name of the process */

	/* p_status.(ready, sending, receiving) */
	int  p_status;	/* process flags. A proc is runnable iff p_status==0*/

	/**
	 * 本进程持有的消息.
	 * SENDING状态:	表示此进程正在发送的消息(还没有目标进程接受,所以此进程阻塞)
	 * RECEIVING状态:表示此进程想要接受的消息(还没有进程发送,所以此进程阻塞)
	 * REARY状态:	p_hold_msg = 0,因为此进程没有消息要处理 
	 */
	MESSAGE * p_hold_msg;

	int p_want_recvfrom;		/* 'p' means index of proc_table[] */
	int p_want_sendto;

 	int has_int_msg;	/**
	 					 * 表示该进程在等待一个硬件中断消息
						 * eg: 硬盘驱动可能会等待硬盘中断的发生,
						 * 	   系统在得知中断发生后, 将此位置设定为'1'
						 */

 	struct Proc *q_sending;		/* pointer to the first process that deliver msg to this process */

 	struct Proc *next_sending;	/* the next process that deliver msg to the destination of this process  */

	int nr_tty;		/* just for simplifying, every Process have its TTY. */
}PROCESS;


/* Task */
typedef struct s_tatask_fsk {
	task_f	initial_eip;
	int		stacksize;
	char	name[32];
}TASK;


/* Number of tasks & procs */
#define NR_TASKS	4
#define NR_PROCS	3	/* user process */
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
#define STACK_SIZE_TOTAL	(STACK_SIZE_TTY + STACK_SIZE_SYS + STACK_SIZE_HD + STACK_SIZE_FS + STACK_SIZE_TESTA + STACK_SIZE_TESTB + STACK_SIZE_TESTC)

/* Process Status*/
#define SENDING   0x02	/* set when proc trying to send */
#define RECEIVING 0x04	/* set when proc trying to recv */

 /* tasks */
#define INVALID_DRIVER	-20
#define INTERRUPT		-10
#define TASK_TTY		0
#define TASK_SYS		1
#define TASK_HD			2
/* #define TASK_FS		3 */
/* #define TASK_MM		4 */
#define ANY			(NR_TASKS + NR_PROCS + 10)
#define NO_TASK		(NR_TASKS + NR_PROCS + 20)

 
#endif