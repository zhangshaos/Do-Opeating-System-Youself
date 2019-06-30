
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            start.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*-------------------------------------------------------------------
路径:kernel/start.c
用途:
	由loader进入kernel后,调整gdt.idt
时间:2019-6-29-Chauncey Zhang
-------------------------------------------------------------------*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "global.h"


/*======================================================================*
                            cstart
用途:1.将gdt从loader中移到kernel中,
	2.初始化idt.
 *======================================================================*/
PUBLIC void cstart()
{
	disp_str("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		 "-----\"cstart\" begins-----\n");

	/* 将 LOADER 中的 GDT 复制到新的 GDT 中 */
	memcpy(&gdt,				  /* New GDT */
	       (void*)(*((u32*)(&gdt_ptr[2]))),   /* Base  of Old GDT */
	       *((u16*)(&gdt_ptr[0])) + 1	  /* Limit of Old GDT */
		);

	/* 创建新的gdt_ptr;
	   gdt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sgdt/lgdt 的参数。*/
	u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
	u32* p_gdt_base  = (u32*)(&gdt_ptr[2]);
	*p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
	*p_gdt_base  = (u32)&gdt;

	/* 创建新的idt_ptr,
	   idt_ptr[6] 共 6 个字节：0~15:Limit  16~47:Base。用作 sidt/lidt 的参数。*/
	u16* p_idt_limit = (u16*)(&idt_ptr[0]);
	u32* p_idt_base  = (u32*)(&idt_ptr[2]);
	*p_idt_limit = IDT_SIZE * sizeof(GATE) - 1;
	*p_idt_base  = (u32)&idt;

	char tmp_str[]="666\n";
	//disp_color_str(tmp_str,0x74);
	//disp_int(1);ok
	disp_str(tmp_str);//如果直接使用字符串"666"作参数,这个调用导致OF
	/* 初始化两片8259A(设置中断向量号0x20...;
	   初始化idt. */
	init_prot();
	//disp_color_str(tmp_str,0x74);
	//disp_int(2);ok
	disp_str(tmp_str);//如果直接使用字符串"666"作参数,这个调用也导致了OF
}
