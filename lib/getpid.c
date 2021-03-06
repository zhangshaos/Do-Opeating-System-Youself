


#include "proto.h"
#include "type.h"
#include "const.h"
#include "log.h"




/*****************************************************************************
 *                                getpid
 *****************************************************************************/
/**
 * Get the PID.
 * 
 * @return The PID.
 *****************************************************************************/
PUBLIC int getpid()
{
	MESSAGE msg;
	msg.type = GET_PID;

	
	send_recv(BOTH, TASK_SYS, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.PID;
}
