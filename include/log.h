/**
 * @brief:  Record for logging.
 *          You can rocord log in memory, and write memory to log files
 *          with bochs' command: "writemem" 
 */
#ifndef __LOG_H__
#define __LOG_H__

#include "stdio.h"
#include "string.h"
#include "proto.h"
#include "type.h"
#include "global.h"


int LOG_SPRINTF(char *buf, const char *fmt, ...);

void LOG_RECORD(const char *fmt,...);

void LOG_NEXT_PROC();

// log function.
// @WARNING
// <ring0>:
//          because write directly memory...
 void LOG_CALLS(PROCESS *p, const char *func_name);

 void LOG_RETS(PROCESS *p, const char *func_name);

 void LOG_IPC(int function,int p_source,int p_dest,MESSAGE *pmsg);



 
 
// <Ring0-3>s
void DEBUG_MEMCPY(char *dest, const char *source, int count);
// <Ring0>
void sys_debug_memcpy(char *dest, const char *source, int count, MESSAGE* unused);

int DEBUG_VSPRINTF(char *buf, const char *fmt, va_list args);

int sys_debug_vsprintf(char *buf, const char *fmt, va_list args, MESSAGE *unused);

// void BREAK_POINT(void * eip);

// void sys_break_point(int unused1, int unused2, void *ea,PROCESS *p);


#endif