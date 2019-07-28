




#include "type.h"
#include "const.h"
#include "struct_proc.h"


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





/* 辅助函数 */
/**
 * 发出严重警告,终止程序
 * 
 */
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
