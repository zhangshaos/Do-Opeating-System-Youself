/*================================================================================================
File name:		kernel/global.c
Description:	*将global.h中的全局变量定义在这里
Copyright:		Chauncey Zhang
Date:		 	2019-6-29
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "fs.h"
#include "tty.h"
#include "console.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "log.h"


PUBLIC	PROCESS	proc_table[NR_TASKS + NR_PROCS];

PUBLIC const	TASK	task_table[NR_TASKS] = {
	{task_tty, STACK_SIZE_TTY, "TTY"},
	{task_sys, STACK_SIZE_SYS, "SYS"},
	{task_hd,  STACK_SIZE_HD,  "HD" },
	{task_fs,  STACK_SIZE_FS,  "FS" },
	{task_mm,  STACK_SIZE_MM,  "MM" }
};

PUBLIC const	TASK	user_proc_table[NR_NATIVE_PROCS] = {
	/* entry    stack size     proc name */
	/* -----    ----------     --------- */
	{Init,   STACK_SIZE_INIT,  "INIT" },
	{TestA,  STACK_SIZE_TESTA, "TestA"},
	{TestB,  STACK_SIZE_TESTB, "TestB"},
	{TestC,  STACK_SIZE_TESTC, "TestC"}};
/* PUBLIC	struct task	user_proc_table[NR_PROCS] = { */
/* 	{TestA, STACK_SIZE_TESTA, "TestA"}, */
/* 	{TestB, STACK_SIZE_TESTB, "TestB"}, */
/* 	{TestC, STACK_SIZE_TESTC, "TestC"}}; */

PUBLIC	char		task_stack[STACK_SIZE_TOTAL];


// 键盘扫描码缓冲区
PUBLIC KB_INPUT	KB_CODE_INBUF;


PUBLIC	TTY			tty_table[NR_CONSOLES];
PUBLIC	CONSOLE		console_table[NR_CONSOLES];

PUBLIC	irq_handler		irq_table[NR_IRQ];

PUBLIC const system_call	sys_call_table[NR_SYS_CALL] = {
	sys_printx,
	sys_sendrec,
	sys_debug_memcpy,
	sys_debug_vsprintf
	// sys_break_point
	};

/* FS related below */
/*****************************************************************************/
/**
 * For dd_map[k],
 * `k' is the device nr.\ dd_map[k].driver_nr is the driver nr.
 *
 * Remeber to modify include/const.h if the order is changed
 *****************************************************************************/
const struct dev_drv_map dd_map[] = {
	/* driver nr.		major device nr.
	   ----------		---------------- */
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
PUBLIC 	u8 *	const	fsbuf		= (u8*)0x600000;
PUBLIC	const int		FSBUF_SIZE	= 0x100000;

/**
 * 7MB~8MB: buffer for MM
 */
PUBLIC	u8 *		mmbuf		= (u8*)0x700000;
PUBLIC	const int	MMBUF_SIZE	= 0x100000;



// log
// logbuf 使用20MB-30MB地址空间
PUBLIC char * const   logbuf      =   (char*)0x1400000; 
PUBLIC const int      LOGBUF_SIZE =   0xa00000;
PUBLIC unsigned int   loglen      =   1;
PUBLIC unsigned int   call_stack_pos   = 1;   