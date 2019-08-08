/*================================================================================================
File name:		kernel/proc.c
Description:	*定义进程调度函数
Copyright:		Chauncey Zhang
Date:		 	2019-7-14
===============================================================================================*/


#include"const.h"
#include"struct_proc.h"
#include"global.h"
#include"func_proto.h"
#include"stdio.h"


/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	PROCESS* p;
	int	 greatest_ticks = 0;

	while (0 == greatest_ticks) 
	{
		/* 寻找剩余ticks(优先级)最多的进程 */
		for (p = proc_table; p < proc_table+NR_TASKS+NR_PROCS; p++) 
		{
			/* 只选择"ready"的进程 */
			if(0 == p->p_status)
			{
				if (p->ticks > greatest_ticks)
				{
					greatest_ticks = p->ticks;
					p_proc_ready = p;
				}
			}
		}

		/* 如果所有进程都结束了, 重新跑一遍 */
		if (0 == greatest_ticks) 
		{
			for (p = proc_table; p < proc_table+NR_TASKS+NR_PROCS; p++) 
			{
				if(0 == p->p_status)
				{
					p->ticks = p->priority;
				}
			}
		}
	}
}






/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	char filename[MAX_FILENAME_LEN+1] = "blah";
	const char bufw[] = "abcde";
	const int rd_bytes = 3;
	char bufr[rd_bytes];

	assert(rd_bytes <= strlen(bufw));

	/* create */
	int fd = open(filename, O_CREAT | O_RDWR);
	assert(fd != -1);
	printf("File created: %s (fd %d)\n", filename, fd);

	/* write */
	int n = write(fd, bufw, strlen(bufw));
	assert(n == strlen(bufw));

	/* close */
	close(fd);

	/* open */
	fd = open(filename, O_RDWR);
	assert(fd != -1);
	printf("File opened. fd: %d\n", fd);

	/* read */
	n = read(fd, bufr, rd_bytes);
	assert(n == rd_bytes);
	bufr[n] = 0;
	printf("%d bytes read: %s\n", n, bufr);

	/* close */
	close(fd);

	char * filenames[] = {"/foo", "/bar", "/baz"};

	/* create files */
	for (int i = 0; i < sizeof(filenames) / sizeof(filenames[0]); i++) 
	{
		fd = open(filenames[i], O_CREAT | O_RDWR);
		assert(fd != -1);
		printf("File created: %s (fd %d)\n", filenames[i], fd);
		close(fd);
	}

	char * rfilenames[] = {"/bar", "/foo", "/baz", "/dev_tty0"};

	/* remove files */
	for (int i = 0; i < sizeof(rfilenames) / sizeof(rfilenames[0]); i++) 
	{
		if (unlink(rfilenames[i]) == 0)
			printf("File removed: %s\n", rfilenames[i]);
		else
			printf("Failed to remove file: %s\n", rfilenames[i]);
	}

	spin("TestA");
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int i = 0x1000;
	while(1){
		printf("B.");
		milli_delay(1000);
	}
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{
	int i = 0x2000;
	while(1){
		printf("C.");
		milli_delay(1000);
	}
}