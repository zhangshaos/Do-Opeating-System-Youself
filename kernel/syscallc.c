




#include "type.h"
#include "const.h"
#include "struct_proc.h"
#include "func_proto.h"


/* 下面是系统调用 */
/**
 * 获得系统ticks
 * 
 */
PUBLIC int get_ticks()
{
    MESSAGE msg;
    reset_msg(&msg);
    msg.type = GET_TICKS;

    send_recv(BOTH, TASK_SYS, &msg);
    return msg.RETVAL;
}
