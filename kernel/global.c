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
#include"struct_tty.h"
#include "struct_hd.h"



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


/* current console's index */
PUBLIC	int		nr_current_console;


/* array of all PCBs */
PUBLIC	PROCESS		proc_table[NR_TASKS + NR_PROCS];


/* descriptions for all Tasks & User-Process */
PUBLIC	TASK		task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "TTY"},
	{task_sys, STACK_SIZE_SYS, "SYS"},
	{task_hd,  STACK_SIZE_HD,  "HD" },
	{task_fs,  STACK_SIZE_FS,  "FS" }
	};

PUBLIC  TASK    user_proc_table[NR_PROCS] = {
	{TestA, STACK_SIZE_TESTA, "TestA"},
	{TestB, STACK_SIZE_TESTB, "TestB"},
	{TestC, STACK_SIZE_TESTC, "TestC"}
	};


/* isolated stack for all Tasks, divided by every Task*/
PUBLIC	char		task_stack[STACK_SIZE_TOTAL];


/* all hardware interruption handler */
PUBLIC	irq_handler	irq_table[NR_IRQ];

/* standard I/O */
PUBLIC	TTY			tty_table[NR_CONSOLES];
PUBLIC	CONSOLE		console_table[NR_CONSOLES];


/* customed system call (int 90h) handler */
PUBLIC	system_call	sys_call_table[NR_SYS_CALL] = {
	sys_printx,	/* I/O moudle has been not part of IPC. */
	sys_sendrec
	};


PUBLIC	struct dev_drv_map dd_map[] = {
	{INVALID_DRIVER},	/**< 0 : Unused */
	{INVALID_DRIVER},	/**< 1 : Reserved for floppy driver */
	{INVALID_DRIVER},	/**< 2 : Reserved for cdrom driver */
	{TASK_HD},			/**< 3 : Hard disk */
	{TASK_TTY},			/**< 4 : TTY */
	{INVALID_DRIVER}	/**< 5 : Reserved for scsi disk driver */
};


 /**
 * 6MB~7MB: buffer for FS
 */
PUBLIC	u8 *				fsbuf		= (u8*)0x600000;
PUBLIC	const int			FSBUF_SIZE	= 0x100000;

PUBLIC	struct file_desc	f_desc_table[NR_FILE_DESC];
PUBLIC	struct inode		inode_table[NR_INODE];
PUBLIC	struct super_block	super_block[NR_SUPER_BLOCK];

PUBLIC	MESSAGE			    fs_msg;
PUBLIC	PROCESS             *pcaller;
PUBLIC	struct inode *		root_inode;
