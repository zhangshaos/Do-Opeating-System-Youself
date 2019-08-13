/**
 * @brief:  Record for logging.
 *          You can rocord log in memory, and write memory to log files
 *          with bochs' command: "writemem" 
 */

#include "stdio.h"
#include "string.h"
#include "proto.h"
#include "type.h"
#include "global.h"
#include "log.h"




// #define WEAK_DEF __attribute__((weak))


// log function.
// @WARNING
// <ring1-3>:
//          because write directly memory...
int LOG_SPRINTF(char *buf, const char *fmt, ...)
{
	va_list arg = (va_list)((char*)(&fmt) + 4);        /* 4 是参数 fmt 所占堆栈中的大小 */
	return DEBUG_VSPRINTF(buf, fmt, arg);
}

void LOG_RECORD(const char *fmt,...)
{
    va_list arg = (va_list)((char*)(&fmt) + 4);
    /* loglen += */ DEBUG_VSPRINTF(logbuf + loglen,fmt,arg);
}

// 这个函数仅仅被restart()调用, 用来记录进程调度后, 执行流的去向
void LOG_NEXT_PROC()
{
    const char *p_name = p_proc_ready->name;
    /* loglen += */ LOG_SPRINTF(logbuf + loglen,"Next:%s\n",p_name);
    assert(loglen < LOGBUF_SIZE);
}

 void LOG_CALLS(PROCESS *p, const char *func_name)
{
    // process status
    const char * status;
    switch (p->p_flags)
    {
    case 0:
        status = "READY";
        break;

    case SENDING:
        status = "SENDING";
        break;

    case RECEIVING:
        status = "RECVING";
        break;

    case WAITING:
        status = "WAITING";
        break;

    case HANGING:
        status = "HANGING";
        break;
    
    default:
        status = "UNKOWN";
        break;
    }

    for(int i = 0;i<call_stack_pos;++i)
    {
        // logbuf[loglen++] = '\t';
        DEBUG_MEMCPY(logbuf + loglen++,"\t",1);
    }
    ++call_stack_pos; //增加下一个函数调用的缩进
    /* loglen += */ LOG_SPRINTF(logbuf + loglen,"%s-%s:%s(\n",p->name,status ,func_name);
    assert(loglen < LOGBUF_SIZE);
}

 void LOG_RETS(PROCESS *p, const char *func_name)
{
    // process status
    const char * status;
    switch (p->p_flags)
    {
    case 0:
        status = "READY";
        break;

    case SENDING:
        status = "SENDING";
        break;

    case RECEIVING:
        status = "RECVING";
        break;

    case WAITING:
        status = "WAITING";
        break;

    case HANGING:
        status = "HANGING";
        break;
    
    default:
        status = "UNKOWN";
        break;
    }

    --call_stack_pos;
    for(int i=0;i<call_stack_pos;++i)
    {
        // logbuf[loglen++]='\t';
        DEBUG_MEMCPY(logbuf + loglen++,"\t",1);
    }
    /* loglen += */ LOG_SPRINTF(logbuf + loglen,")%s(%s-%s)\n",func_name,p->name,status);
    assert(loglen < LOGBUF_SIZE);
}

 void LOG_IPC(int function,int p_source,int p_dest,MESSAGE *pmsg)
{
    // const char *source_name = p_source < NR_TASKS ? task_table[p_source].name : user_proc_table[p_source-NR_TASKS].name;
    const char *source_name,*dest_name;

    // assert(p_source>=0);
    if(p_source >= 0 && p_source < NR_TASKS + NR_PROCS)
    {
        source_name = proc_table[p_source].name;
    }
    else
    {
        switch (p_source)
        {
        case INTERRUPT:
            source_name = "INT";
            break;
        
        case ANY:
            source_name = "ANY";
            break;

        default:
            assert(0&&"bad IPC type");
            break;
        }
    }

    // assert(p_dest>=0);
    if(p_dest >= 0 && p_dest < NR_TASKS + NR_PROCS)
    {
        dest_name = proc_table[p_dest].name;
    }
    else
    {
        switch (p_dest)
        {
        case INTERRUPT:
            dest_name = "INT";
            break;
        
        case ANY:
            dest_name = "ANY";
            break;

        default:
            assert(0&&"bad IPC type");
            break;
        }
    }

    const char *dest_status;
    switch (proc_table[p_dest].p_flags)
    {
    case 0:
        dest_status = "READY";
        break;

    case SENDING:
        dest_status = "SENDING";
        break;

    case RECEIVING:
        dest_status = "RECVING";
        break;

    case WAITING:
        dest_status = "WAITING";
        break;

    case HANGING:
        dest_status = "HANGING";
        break;
    
    default:
        dest_status = "UNKOWN";
        break;
    }
    

    const char *func_name;
    switch (function)
    {
    case BOTH:
        func_name = "SEND&RECV";
        break;
    
    case SEND:
        func_name = "SEND";
        break;

    case RECEIVE:
        func_name = "RECV";
        break;
    
    default:
        assert(0);
        break;
    }

    const char* msg_type;
    MESSAGE *msg = va2la(p_source,(void*)pmsg);
    switch(msg->type)
    {
    case HARD_INT:
        msg_type = "INT";
        break;

// SYS task
    case GET_TICKS:
        msg_type = "GET_TICKS";
        break;
    
    case GET_PID:
        msg_type = "GET_PID";
        break;

    case GET_RTC_TIME:
        msg_type = "GET_RTC_TYPE";
        break;

// FS
            case OPEN:
        msg_type = "OPEN";
        break;

            case CLOSE:
        msg_type = "CLOSE";
        break;

            case READ:
        msg_type = "READ";
        break;

            case WRITE:
        msg_type = "WRITE";
        break;

            case LSEEK:
        msg_type = "LSEEK";
        break;

            case STAT:
        msg_type = "STAT";
        break;

            case UNLINK:
        msg_type = "UNLINK";
        break;

// FS & TTY
            case SUSPEND_PROC:
        msg_type = "SUSPEND_PORC";
        break;

            case RESUME_PROC:
        msg_type = "RESUME_PROC";
        break;

// MM
            case EXEC:
        msg_type = "EXEC";
        break;

            case WAIT:
        msg_type = "WAIT";
        break;

// FS & MM
            case FORK:
        msg_type = "FORK";
        break;

            case EXIT:
        msg_type = "EXIT";
        break;

// TTY, SYS, FS, MM, ...
        case SYSCALL_RET:
        msg_type = "SYSCALL_RET";
        break;

// DRIVERS
            case DEV_OPEN:
        msg_type = "DEV_OPEN";
        break;

            case DEV_CLOSE:
        msg_type = "DEC_CLOSE";
        break;

            case DEV_READ:
        msg_type = "DEV_READ";
        break;

            case DEV_WRITE:
        msg_type = "DEV_WRITE";
        break;

            case DEV_IOCTL:
        msg_type = "DEV_IOCTL";
        break;
    
    default:
        msg_type = "UNKNOWN";
        break;
    }

    // write logs...
    for(int i=0;i<call_stack_pos;++i)
    {
        // logbuf[loglen++]='\t';
        DEBUG_MEMCPY(logbuf + loglen++,"\t",1);
    }
    /* loglen += */ LOG_SPRINTF(logbuf + loglen,"[%s %s %s(%s) : %s]\n",source_name,func_name,dest_name, dest_status,msg_type);
    assert(loglen < LOGBUF_SIZE);
}










// <Ring0>
void sys_debug_memcpy(char *dest, const char *source, int count, MESSAGE * unused)
{
    memcpy( (void*)dest,(void*)source, count );
}

int sys_debug_vsprintf(char *buf, const char *fmt, va_list args, MESSAGE *unused)
{
    int ret = vsprintf(buf,fmt,args);
    loglen += ret;  //提前增加loglen,否则restart(ring0->ring1-3)时,LOG_NEXT_PROC()会重复读写loglen长度.
   return ret;
}

// void sys_break_point(int unused1, int unused2, void *ea,PROCESS *p)
// {
//     void * laddr = va2la(p-proc_table,ea);
//     asm volatile (
//         "movl %0,%%eax\n\t"
//         "movl %%eax,%%dr0\n\t"
//         "movl $0x00000202,%%eax\n\t"
//         "movl %%eax,%%dr7\n\t"
//         :
//         :"r"(laddr)
//         :"%eax"
//     );
// }