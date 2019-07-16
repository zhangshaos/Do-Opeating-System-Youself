/*================================================================================================
File name:		kernel/global.c
Description:	*全局变量定义处
Copyright:		Chauncey Zhang
Date:		 	2019-7-15
===============================================================================================*/


#include"const.h"
#include"type.h"
#include"struct_descript.h"
#include"struct_proc.h"
#include"const_interrupt.h"
#include"func_proto.h"



/* system clock offered by 8254 chips.(every tick means about 10ms)*/
PUBLIC	int		ticks;


/* the position of writting directly video memory. */
PUBLIC	int		disp_pos;


/* GDTR(loaded by lgdt) and GDT */
PUBLIC	u8		    gdt_ptr[6];	/* 0~15:Limit  16~47:Base */
PUBLIC	DESCRIPTOR	gdt[GDT_SIZE];

/* IDTR(loaded by lidt) and IDT */
PUBLIC	u8		    idt_ptr[6];	/* 0~15:Limit  16~47:Base */
PUBLIC	GATE		idt[IDT_SIZE];


/* 0:not reenter interupting or exception, -1 or other:reenter.*/
PUBLIC	u32		k_reenter;


/* task  status segement */
PUBLIC	TSS		tss;


/* point to the ready process table for invoking*/
PUBLIC	PROCESS*	p_proc_ready;


/* array of all PCBs */
PUBLIC	PROCESS		proc_table[NR_TASKS];


/* descriptions for all Tasks */
PUBLIC	TASK		task_table[NR_TASKS] = {
	{TestA, STACK_SIZE_TESTA, "TestA"},
	{TestB, STACK_SIZE_TESTB, "TestB"},
	{TestC, STACK_SIZE_TESTC, "TestC"}
	};

/* isolated stack for all Tasks, divided by every Task*/
PUBLIC	char		task_stack[STACK_SIZE_TOTAL];


/* all hardware interruption handler */
PUBLIC	irq_handler	irq_table[NR_IRQ];


/* customed system call (int 90h) handler */
PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {
	sys_get_ticks
	};

