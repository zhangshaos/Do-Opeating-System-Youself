/*************************************************************************//**
 *****************************************************************************
 * @file   stat.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   Wed May 21 21:17:21 2008
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "const.h"
#include "proto.h"


/*****************************************************************************
 *                                stat
 *************************************************************************//**
 * 获取文件(stat):大小...信息
 * 
 * @param path 
 * @param buf 
 * 
 * @return  On success, zero is returned. On error, -1 is returned.
 *****************************************************************************/
PUBLIC int stat(const char *path, struct stat *buf)
{
	MESSAGE msg;

	msg.type		= STAT;
	msg.PATHNAME	= (void*)path;
	msg.BUF			= (void*)buf;
	msg.NAME_LEN	= strlen(path);

	send_recv(BOTH, TASK_FS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.RETVAL;
}
