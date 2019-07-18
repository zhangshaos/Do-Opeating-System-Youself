/*================================================================================================
File name:		include/global.h
Description:	*定义全局变量
                (如果定义了宏GLOBAL_VARIABLES_HERE,则该编译单元中全局变量视为global;否则视为external)
Copyright:		Chauncey Zhang
Date:		 	2019-6-29
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

#include"tty.h"

EXTERN	int		ticks;		/* system clock offered by 8254 chips.*/

EXTERN	int		    disp_pos;
EXTERN	u8		    gdt_ptr[6];	/* 0~15:Limit  16~47:Base */
EXTERN	DESCRIPTOR	gdt[GDT_SIZE];
EXTERN	u8		    idt_ptr[6];	/* 0~15:Limit  16~47:Base */
EXTERN	GATE		idt[IDT_SIZE];

EXTERN	u32		k_reenter;	/* 0:not reenter interupting or exception, -1 or other:reenter.*/

EXTERN	TSS		tss;                /* task  status segement */
EXTERN	PROCESS*	p_proc_ready;	/* point to the ready process table for invoking*/

EXTERN	int		nr_current_console;

extern	PROCESS		proc_table[];	/* array of PCB */
extern	char		task_stack[];	/* stack that can be divided for all tasks */

extern  TASK        task_table[];	/* array of description for all tasks */
extern  TASK        user_proc_table[];
extern	irq_handler	irq_table[];	/* array of interupt handler */
extern	TTY		    tty_table[];    /* TTY contains public KEYBOARD(input) and private CONSOLE(ouput) */
extern  CONSOLE     console_table[];/* display characters on screen */
