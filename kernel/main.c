/*================================================================================================
File name:		kernel/main.c
Description:	*kernel_main():初始化个进程的PCB,跳转到任务进程
				*定义三个任务
Copyright:		Chauncey Zhang
Date:		 	2019-7-14
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "stdio.h"
#include "log.h"

/*======================================================================*
                            kernel_main
	===================================================================
							初始化化各进程的PCB
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	int i, j, eflags, prio;
        u8  rpl;
        u8  priv; /* privilege */

	TASK * t;
	PROCESS * p = proc_table;

	char * stk = task_stack + STACK_SIZE_TOTAL;

	// 填充 PCB
	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++,t++) {
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p->p_flags = FREE_SLOT;
			continue;
		}

	        if (i < NR_TASKS) {     /* TASK */
                        t		= task_table + i;
                        priv	= PRIVILEGE_TASK;	//描述符属性
                        rpl     = RPL_TASK;	//选择符属性
                        eflags  = 0x1202;	/* IF=1, IOPL=1, bit 2 is always 1 */
						prio    = 15;
                }
                else {                  /* USER PROC */
                        t		= user_proc_table + (i - NR_TASKS);
                        priv	= PRIVILEGE_USER;	//描述符属性
                        rpl     = RPL_USER;	//选择符属性
                        eflags  = 0x202;	/* IF=1, bit 2 is always 1 */
						prio    = 5;
                }

		strcpy(p->name, t->name);	/* name of the process */
		p->p_parent = NO_TASK;

		if (strcmp(t->name, "INIT") != 0) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p->ldts[INDEX_LDT_C].attr1  = DA_C   | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		}
		else {		/* INIT process */
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_desc(&p->ldts[INDEX_LDT_C],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_desc(&p->ldts[INDEX_LDT_RW],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
			/**
			 * @Note:
			 * We use 0x0 as cs and d/s/es 's base addr here,
			 * which is a convenient issue.
			 * And we adjust child INIT's LDT(cs and d/s/es) at MM::do_fork()
			 */
		}

		p->regs.cs = INDEX_LDT_C << 3 |	SA_TIL | rpl;
		p->regs.ds =
			p->regs.es =
			p->regs.fs =
			p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip	= (u32)t->initial_eip;
		p->regs.esp	= (u32)stk;
		p->regs.eflags	= eflags;

		p->ticks = p->priority = prio;

		p->p_flags = 0;
		p->p_hold_msg = 0;
		p->p_recvfrom = NO_TASK;
		p->p_sendto = NO_TASK;
		p->has_int_msg = 0;
		p->q_sending = 0;
		p->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p->filp[j] = 0;

		stk -= t->stacksize;
	}

	k_reenter = 0;
	ticks = 0;

	/* 准备进程调度 */
	p_proc_ready	= proc_table;

	/* 准备时钟中断 */
	init_clock();
	/* 准备键盘中断 */
    init_keyboard();


// 测试代码,<Ring0>环境下
// DEBUG_MEMCPY((char*)0x1400000,"\t",1);//测试结束,OK
	// BREAK_POINT((void*)restart);//这个没用...
	// LOG_RECORD("test_ring0 is ok?");//ring0 is ok...

	/* ring0->ring1,开始执行任务 */
	restart();

	disp_str("\n!!!Process Over!!!\n"); /* in common, you can't come on here. */
	while(1)
	{
		/* nop */
	}
}




// /*======================================================================*
//                                TestA
//  *======================================================================*/
// void TestA()
// {
// 	char filename[MAX_FILENAME_LEN+1] = "blah";
// 	const char bufw[] = "abcde";
// 	const int rd_bytes = 3;
// 	char bufr[rd_bytes];

// 	assert(rd_bytes <= strlen(bufw));

// 	/* create */
// 	int fd = open(filename, O_CREAT | O_RDWR);
// 	assert(fd != -1);
// 	printl("File created: %s (fd %d)\n", filename, fd);

// 	/* write */
// 	int n = write(fd, bufw, strlen(bufw));
// 	assert(n == strlen(bufw));

// 	/* close */
// 	close(fd);

// 	/* open */
// 	fd = open(filename, O_RDWR);
// 	assert(fd != -1);
// 	printl("File opened. fd: %d\n", fd);

// 	/* read */
// 	n = read(fd, bufr, rd_bytes);
// 	assert(n == rd_bytes);
// 	bufr[n] = 0;
// 	printl("%d bytes read: %s\n", n, bufr);

// 	/* close */
// 	close(fd);

// 	char * filenames[] = {"/foo", "/bar", "/baz"};

// 	/* create files */
// 	for (int i = 0; i < sizeof(filenames) / sizeof(filenames[0]); i++) 
// 	{
// 		fd = open(filenames[i], O_CREAT | O_RDWR);
// 		assert(fd != -1);
// 		printl("File created: %s (fd %d)\n", filenames[i], fd);
// 		close(fd);
// 	}

// 	char * rfilenames[] = {"/bar", "/foo", "/baz", "/dev_tty0"};

// 	/* remove files */
// 	for (int i = 0; i < sizeof(rfilenames) / sizeof(rfilenames[0]); i++) 
// 	{
// 		if (unlink(rfilenames[i]) == 0)
// 			printl("File removed: %s\n", rfilenames[i]);
// 		else
// 			printl("Failed to remove file: %s\n", rfilenames[i]);
// 	}

// 	spin("TestA");
// }

// /*======================================================================*
//                                TestB
//  *======================================================================*/
// void TestB()
// {
// 	char tty_name[] = "/dev_tty1";

// 	int fd_stdin  = open(tty_name, O_RDWR);
// 	assert(fd_stdin  == 0);
// 	int fd_stdout = open(tty_name, O_RDWR);
// 	assert(fd_stdout == 1);

// 	char rdbuf[128];

// 	while (1) 
// 	{
// 		printf("$ ");
// 		int r = read(fd_stdin, rdbuf, 70);
// 		rdbuf[r] = 0;

// 		if (strcmp(rdbuf, "hello") == 0)
// 			printf("hello world!\n");
// 		else
// 			if (rdbuf[0])
// 				printf("{%s}\n", rdbuf);
// 	}

// 	assert(0); /* never arrive here */
// }

// /*======================================================================*
//                                TestC
//  *======================================================================*/
// void TestC()
// {
// 	char tty_name[] = "/dev_tty2";

// 	int fd_stdin  = open(tty_name, O_RDWR);
// 	assert(fd_stdin  == 0);
// 	int fd_stdout = open(tty_name, O_RDWR);
// 	assert(fd_stdout == 1);

// 	while(1)
// 	{
// 		printf("C");
// 		milli_delay(1000);
// 	}
// }


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};

/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 * 
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char * filename)
{
	printf("[extract `%s'\n", filename);
	int fd = open(filename, O_RDWR);
	assert(fd != -1);

	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);

	while (1) 
	{
		// 读取tar文件头到buf
		read(fd, buf, SECTOR_SIZE);
		if (buf[0] == 0)
			break;

		// tar包实际上就是一个(512字节的头文件+文件)+(头文件+文件)+...叠放在一起
		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
		{
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */
		}

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWR);
		if (fdout == -1) 
		{
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]");
			return;
		}
		printf("    %s (%d bytes)\n", phdr->name, f_len);
		while (bytes_left) 
		{
			int iobytes = min(chunk, bytes_left);
			// 读取tar包中放在文件头后面的真实文件
			read(fd, buf, ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			// 将解tar包后的真实文件写回硬盘中
			write(fdout, buf, iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);
	}

	close(fd);

	printf(" done]\n");
}

/*****************************************************************************
 *                                shabby_shell
 *****************************************************************************/
/**
 * A very very simple shell.
 * 
 * @param tty_name  TTY file name.
 *****************************************************************************/
void shabby_shell(const char * tty_name)
{
	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];

	while (1) 
	{
		write(1, "$ ", 2);
		int r = read(0, rdbuf, 70);
		rdbuf[r] = 0;

		int argc = 0;
		char * argv[PROC_ORIGIN_STACK];
		char * p_cmd = rdbuf;
		char * tmp;
		int word = 0; // 表示现在p_cmd指向字符串(用户键入shell的参数)
		char ch;
		do {
			ch = *p_cmd;
			if (*p_cmd != ' ' && *p_cmd != 0 && !word) {
				// 当p_cmd指向键入shell的参数时
				tmp = p_cmd;
				word = 1;
			}
			if ((*p_cmd == ' ' || *p_cmd == 0) && word) {
				// 当p_cmd扫描完整个字符串参数后
				word = 0;
				argv[argc++] = tmp;
				*p_cmd = 0;
			}
			p_cmd++;
		} while(ch);
		
		argv[argc] = 0;

		int fd = open(argv[0], O_RDWR);// 打开cmd命令所对应的文件
		if (fd == -1) {
			if (rdbuf[0]) {
				write(1, "{", 1);
				write(1, rdbuf, r);
				write(1, "}\n", 2);
			}
		}
		else {
			// Q:为什么打开后立即关闭?
			// A:打开关闭只是为了测试cmd命令是否存在
			close(fd);
			int pid = fork();
			if (pid != 0) { /* parent */
				int s;
				wait(&s);
			}
			else {	/* child */
				execv(argv[0], argv);
			}
		}
	}

	close(1);
	close(0);
}

/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 * 
 *****************************************************************************/
void Init()
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	printf("Init() is running ...\n");

	/* extract `cmd.tar' */
	untar("/cmd.tar");
			

	char * tty_list[] = {"/dev_tty1", "/dev_tty2"};

	for (int i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) 
	{
		int pid = fork();
		if (pid != 0) { /* parent process */
			printf("[parent is running, child pid:%d]\n", pid);
		}
		else {	/* child process */
			printf("[child is running, pid:%d]\n", getpid());
			// 子进程不继承父进程的打开文件
			close(fd_stdin);
			close(fd_stdout);
			
			shabby_shell(tty_list[i]);
			assert(0);
		}
	}

	while (1) 
	{
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}

	assert(0);

}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	for(;;);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int stdin = open("/dev_tty1",O_RDWR);
	assert(stdin == 0);
	int stdout = open("/dev_tty1",O_RDWR);
	assert(stdout == 1);

	char buf[128] = { '\0' };

	while (1)
	{
		write(stdout,"$ ",2);
		int ret = read(stdin,buf,127);
		buf[ret] = '\0';

		write(stdout,"BACK:",5);
		write(stdout,buf,ret);
		write(stdout,"\n",1);
	}
	
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{
	for(;;);
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}
