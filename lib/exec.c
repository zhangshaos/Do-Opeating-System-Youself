/*************************************************************************//**
 *****************************************************************************
 * @file   lib/exec.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   Tue May  6 14:26:09 2008
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "const.h"
#include "proc.h"
#include "proto.h"

/*****************************************************************************
 *                                exec
 *****************************************************************************/
/**
 * Executes the program pointed by path.
 * 
 * @param path  The full path of the file to be executed.
 * 
 * @return  Zero if successful, otherwise -1.
 *****************************************************************************/
PUBLIC int exec(const char * path)
{
	MESSAGE msg;
	msg.type	= EXEC;
	msg.PATHNAME	= (void*)path;
	msg.NAME_LEN	= strlen(path);
	msg.BUF		= 0;
	msg.BUF_LEN	= 0;

	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.RETVAL;
}

/*****************************************************************************
 *                                execl
 *****************************************************************************/
PUBLIC int execl(const char *path, const char *arg, ...)
{
	va_list parg = (va_list)(&arg);
	char **p = (char**)parg;
	return execv(path, p);
}

/*****************************************************************************
 *                                execv
 *****************************************************************************/
PUBLIC int execv(const char *path, char * argv[])
{
	char **p_str = argv;
	char arg_stack[PROC_ORIGIN_STACK];
	int stack_len = 0;

	while(*p_str++) 
	{
		assert(stack_len + 2 * sizeof(char*) < PROC_ORIGIN_STACK);
		// 计算参数argv[]中传递过来的参数个数
		stack_len += sizeof(char*);
	}

	*((int*)(&arg_stack[stack_len])) = 0;
	stack_len += sizeof(char*);

	char ** q = (char**)arg_stack;
	for (p_str = argv; *p_str != 0; p_str++) 
	{
		// 将每个字符串的地址写入指针数组的正确位置
		*q++ = &arg_stack[stack_len];

		assert(stack_len + strlen(*p_str) + 1 < PROC_ORIGIN_STACK);
		// 将字符串复制到arg_stack[]中
		strcpy(&arg_stack[stack_len], *p_str);
		stack_len += strlen(*p_str);
		arg_stack[stack_len] = 0;
		stack_len++;
	}

	MESSAGE msg;
	msg.type		= EXEC;
	msg.PATHNAME	= (void*)path;
	msg.NAME_LEN	= strlen(path);
	msg.BUF			= (void*)arg_stack;
	msg.BUF_LEN		= stack_len;

	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.RETVAL;
}

