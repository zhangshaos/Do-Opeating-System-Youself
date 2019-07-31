



#include "type.h"
#include "const.h"
#include "struct_proc.h"
#include "func_proto.h"
#include "global.h"



/*****************************************************************************
 *                                task_fs
 *****************************************************************************/
/**
 * <Ring 1> The main loop of TASK FS.
 * 
 *****************************************************************************/
PUBLIC void task_fs()
{
	printf("Task_FS begins.\n");

 	/* open the device: hard disk */
	MESSAGE driver_msg;
	driver_msg.type = DEV_OPEN;
	driver_msg.DEVICE = MINOR(ROOT_DEV);

 	assert(dd_map[MAJOR(ROOT_DEV)].driver_nr != INVALID_DRIVER);

 	send_recv(BOTH, dd_map[MAJOR(ROOT_DEV)].driver_nr, &driver_msg);

 	spin("TASK_FS");
}