/*================================================================================================
File name:		include/global.h
Description:	*全局变量(所有在global.c中定义的变量)申明
Copyright:		Chauncey Zhang
Date:		 	2019-7-15
===============================================================================================*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include"struct_descript.h"
#include"struct_proc.h"
#include"type.h"
#include"struct_tty.h"
#include "struct_hd.h"

/* system clock offered by 8254 chips.(every tick means about 10ms)*/
extern	int		ticks;

/* the position of writting directly video memory. */
extern	int		disp_pos;

/* GDTR(loaded by lgdt) and GDT */
extern	u8		    gdt_ptr[];	/* 0~15:Limit  16~47:Base */
extern	DESCRIPTOR	gdt[];

/* IDTR(loaded by lidt) and IDT */
extern	u8		    idt_ptr[];	/* 0~15:Limit  16~47:Base */
extern	GATE		idt[];

/* 0:not reenter interupting or exception, -1 or other:reenter.*/
extern	u32		k_reenter;

/* task  status segement */
extern	TSS		tss;

/* point to the ready process table for invoking*/
extern	PROCESS*	p_proc_ready;

/* current console's index */
extern	int		nr_current_console;

/* array of all PCBs */
extern	PROCESS		proc_table[];

/* descriptions for all Tasks and User-Process */
extern	TASK		task_table[];
extern  TASK        user_proc_table[];

/* isolated stack for all Tasks, divided by every Task*/
extern	char		task_stack[];

/* all hardware interruption handler */
extern	irq_handler	irq_table[];

/* customed system call (int 90h) handler */
extern	system_call	sys_call_table[];

/* standard I/O */
extern	TTY		    tty_table[];    /* TTY contains public KEYBOARD(input) and private CONSOLE(ouput) */
extern  CONSOLE     console_table[];/* display characters on screen */

/* scan code -> kaymap[] -> key */
extern  u32     keymap[];           /* this array locates in "keymap.c" for its great scale... */

/* fs */
extern	struct file_desc	f_desc_table[];
extern	struct inode		inode_table[];
extern	struct super_block	super_block[];
extern	u8 *			    fsbuf;
extern	const int		    FSBUF_SIZE;
extern	MESSAGE			    fs_msg;
extern	PROCESS             *pcaller;
extern	struct inode *		root_inode;
extern	struct dev_drv_map	dd_map[];

#endif