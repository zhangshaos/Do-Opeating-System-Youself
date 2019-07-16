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

/* array of all PCBs */
extern	PROCESS		proc_table[];

/* descriptions for all Tasks */
extern	TASK		task_table[];

/* isolated stack for all Tasks, divided by every Task*/
extern	char		task_stack[];

/* all hardware interruption handler */
extern	irq_handler	irq_table[];

/* customed system call (int 90h) handler */
extern	system_call	sys_call_table[];


#endif