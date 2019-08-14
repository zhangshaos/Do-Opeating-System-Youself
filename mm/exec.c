/*************************************************************************//**
 *****************************************************************************
 * @file   mm/exec.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   Tue May  6 14:14:02 2008
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

#include <elf.h>


/*****************************************************************************
 *                                do_exec
 *****************************************************************************/
/**
 * Perform the exec() system call.
 * 
 * @return  Zero if successful, otherwise -1.
 *****************************************************************************/
PUBLIC int do_exec()
{
	/*@1: get parameters from the task_mm message */
	int name_len 	= mm_msg.NAME_LEN;	/* length of filename */
	int src 		= mm_msg.source;	/* caller proc nr. */
	assert(name_len < MAX_PATH);

	char pathname[MAX_PATH];
	phys_copy((void*)va2la(TASK_MM, pathname),
		  (void*)va2la(src, mm_msg.PATHNAME),
		  name_len);
	pathname[name_len] = 0;	/* terminate the string */

	/*@2: get the file size */
	struct stat s;
	int ret = stat(pathname, &s);
	if (ret != 0) 
	{
		printl("{MM} MM::do_exec()::stat() returns error. %s", pathname);
		return -1;
	}

	/*@3: read the file */
	int fd = open(pathname, O_RDWR);
	if (fd == -1)
		return -1;
	assert(s.st_size < MMBUF_SIZE);
	read(fd, mmbuf, s.st_size);
	close(fd);

	/*@4: overwrite the current proc image with the new one */
	Elf32_Ehdr* elf_hdr = (Elf32_Ehdr*)(mmbuf);
	for (int i = 0; i < elf_hdr->e_phnum; i++) 
	{
		Elf32_Phdr* prog_hdr = (Elf32_Phdr*)(mmbuf + elf_hdr->e_phoff +
			 					(i * elf_hdr->e_phentsize));
		if (prog_hdr->p_type == PT_LOAD) 
		{
			assert(prog_hdr->p_vaddr + prog_hdr->p_memsz < PROC_IMAGE_SIZE_DEFAULT);
			phys_copy((void*)va2la(src, (void*)prog_hdr->p_vaddr),
				  		(void*)va2la(TASK_MM, mmbuf + prog_hdr->p_offset),
						prog_hdr->p_filesz);
		}
	}

	/*@5: setup the arg stack */
	int orig_stack_len = mm_msg.BUF_LEN;
	char stackcopy[PROC_ORIGIN_STACK];
	phys_copy((void*)va2la(TASK_MM, stackcopy),
		  (void*)va2la(src, mm_msg.BUF),
		  orig_stack_len);

// 为什么进程使用这个栈, 不应该使用自己的进程栈吗?
// 而且, 这个栈貌似是多个进程公用!
	u8 * orig_stack = (u8*)(PROC_IMAGE_SIZE_DEFAULT - PROC_ORIGIN_STACK);

	//@delta:用来重定位参数栈中的字符串指针
	int delta = (int)orig_stack - (int)mm_msg.BUF;

	int argc = 0;
	if (orig_stack_len) 
	{
		char **q = (char**)stackcopy;
		for (; *q != 0; q++,argc++)
			*q += delta;
	}

	phys_copy((void*)va2la(src, orig_stack),
		  (void*)va2la(TASK_MM, stackcopy),
		  orig_stack_len);

	// @6: 为执行程序的eax和ecx赋值(执行程序的_start中将eax和ecx压栈后调用main)
	proc_table[src].regs.ecx = argc; /* argc */
	proc_table[src].regs.eax = (u32)orig_stack; /* argv */

	// @9 setup eip & esp 
	proc_table[src].regs.eip = elf_hdr->e_entry; /* @see _start.asm */
	proc_table[src].regs.esp = PROC_IMAGE_SIZE_DEFAULT - PROC_ORIGIN_STACK;

	// @10: 进程名字
	strcpy(proc_table[src].name, pathname);

	return 0;
}
