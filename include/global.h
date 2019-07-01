/*================================================================================================
File name:		include/global.h
Description:	*定义全局变量
                (如果定义了宏GLOBAL_VARIABLES_HERE,则该编译单元中全局变量视为global;否则视为external)
Copyright:		Chauncey Zhang
Date:		 	2019-6-29
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

EXTERN	int		disp_pos;
EXTERN	u8		gdt_ptr[6];	/* 0~15:Limit  16~47:Base */
EXTERN	DESCRIPTOR	gdt[GDT_SIZE];
EXTERN	u8		idt_ptr[6];	/* 0~15:Limit  16~47:Base */
EXTERN	GATE		idt[IDT_SIZE];
