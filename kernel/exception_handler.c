/*================================================================================================
File name:		kernel/exception.c
Description:	*定义异常处理handler,中断处理参考interrupt.asm
Copyright:		Chauncey Zhang
Date:		 	2019-7-16
===============================================================================================*/


#include"const.h"
#include"global.h"
#include"func_proto.h"


/*======================================================================*
                            exception_handler
 *----------------------------------------------------------------------*
 异常处理
 *======================================================================*/
PUBLIC void exception_handler(
	int vec_no,
	int err_code,
	int eip,
	int cs,
	int eflags)
{
	char err_description[][64] = {	"#DE Divide Error",
					"#DB RESERVED",
					"—  NMI Interrupt",
					"#BP Breakpoint",
					"#OF Overflow",
					"#BR BOUND Range Exceeded",
					"#UD Invalid Opcode (Undefined Opcode)",
					"#NM Device Not Available (No Math Coprocessor)",
					"#DF Double Fault",
					"    Coprocessor Segment Overrun (reserved)",
					"#TS Invalid TSS",
					"#NP Segment Not Present",
					"#SS Stack-Segment Fault",
					"#GP General Protection",
					"#PF Page Fault",
					"—  (Intel reserved. Do not use.)",
					"#MF x87 FPU Floating-Point Error (Math Fault)",
					"#AC Alignment Check",
					"#MC Machine Check",
					"#XF SIMD Floating-Point Exception"
				};

	/* 通过打印空格的方式清空屏幕的前五行，并把 disp_pos 清零 */
	disp_pos = 0;
	for(int i=0;i<80*5;i++)
	{
		disp_str(" ");
	}

	disp_pos = 0;
	int text_color = 0x74; /* 灰底红字 */

	disp_color_str("Exception! --> ", text_color);
	disp_color_str(err_description[vec_no], text_color);
	disp_color_str("\n\n", text_color);
	disp_color_str("EFLAGS:", text_color);
	disp_int(eflags);
	disp_color_str("CS:", text_color);
	disp_int(cs);
	disp_color_str("EIP:", text_color);
	disp_int(eip);

	if(err_code != 0xFFFFFFFF)
	{
		disp_color_str("Error code:", text_color);
		disp_int(err_code);
	}
}