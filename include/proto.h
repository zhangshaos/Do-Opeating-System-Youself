
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/********************************************************************
路径:include/proto.h
用途:
    包含涉及到I/O操作的函数原型.
时间:2019-6-29-Chauncey Zhang
********************************************************************/

/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);
PUBLIC void	init_prot();
PUBLIC void	init_8259A();

/* 这里是不是缺少了void disp_int(int i)和char * itoa(char *str, int num )
   函数定义在lib/klib.c中 */
PUBLIC char * itoa(char * str, int num);
PUBLIC void disp_int(int input);