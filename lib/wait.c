/*************************************************************************//**
 *****************************************************************************
 * @file   wait.c
 * @brief  wait()
 * @author Forrest Y. Yu
 * @date   Tue May 20 01:23:58 2008
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "proto.h"


/*****************************************************************************
 *                                wait
 *****************************************************************************/
/**
 * Wait for the child process to terminiate.
 * 
 * @param status  The value returned from the child.
 * 
 * @return  PID of the terminated child.
 *****************************************************************************/
PUBLIC int wait(int * status)
{
	MESSAGE msg;
	msg.type   = WAIT;

	send_recv(BOTH, TASK_MM, &msg);

	*status = msg.STATUS;

	return (msg.PID == NO_TASK ? -1 : msg.PID);
}
