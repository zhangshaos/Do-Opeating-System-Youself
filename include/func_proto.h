/*================================================================================================
File name:		include/func_proto.h
Description:	*函数原型
Copyright:		Chauncey Zhang
Date:		 	2019-7-16
===============================================================================================*/

#include"const.h"
#include"type.h"
#include"struct_tty.h"
#include"struct_proc.h"



/* clock.c */
PUBLIC void clock_handler(int irq);
PUBLIC void milli_delay(int milli_sec);
PUBLIC int  sys_get_ticks();
PUBLIC void init_clock();


/* keyboard.c */
PUBLIC void init_keyboard();
PUBLIC void keyboard_read(TTY* p_tty);


/* tty.c */
PUBLIC void task_tty();
PUBLIC void respond_key(TTY* p_tty, u32 key);

/* console.c */
PUBLIC void out_char(CONSOLE* p_con, char ch);
PUBLIC void scroll_screen(CONSOLE* p_con, int direction);
PUBLIC void init_screen(TTY* p_tty);
PUBLIC void select_console(int nr_console);
PUBLIC int  is_current_console(CONSOLE* p_con);

/* printf.c */
PUBLIC  int     printf(const char *fmt, ...);

/* vsprintf.c */
PUBLIC  int     vsprintf(char *buf, const char *fmt, va_list args);


/* exception_handler.c */
PUBLIC void exception_handler(int vec_no,int err_code,int eip,int cs,int eflags);


/* init_idt.c */
PUBLIC void spurious_irq(int irq);
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void init_idt();


/* interrupt.asm */
PUBLIC	void	restart();		/* 中断返回 */


PUBLIC 	void	divide_error();			/* 异常处理 */
PUBLIC	void	single_step_exception();
PUBLIC	void	nmi();					/* 不可屏蔽中断 */
PUBLIC	void	breakpoint_exception();
PUBLIC	void	overflow();
PUBLIC	void	bounds_check();
PUBLIC	void	inval_opcode();
PUBLIC	void	copr_not_available();
PUBLIC	void	double_fault();
PUBLIC	void	copr_seg_overrun();
PUBLIC	void	inval_tss();
PUBLIC	void	segment_not_present();
PUBLIC	void	stack_exception();
PUBLIC	void	general_protection();
PUBLIC	void	page_fault();
PUBLIC	void	copr_error();
PUBLIC	void    hwint00();				/* 自定义中断(向量号从0x20开始) */
PUBLIC	void    hwint01();
PUBLIC	void    hwint02();
PUBLIC	void    hwint03();
PUBLIC	void    hwint04();
PUBLIC	void    hwint05();
PUBLIC	void    hwint06();
PUBLIC	void    hwint07();
PUBLIC	void    hwint08();
PUBLIC	void    hwint09();
PUBLIC	void    hwint10();
PUBLIC	void    hwint11();
PUBLIC	void    hwint12();
PUBLIC	void    hwint13();
PUBLIC	void    hwint14();
PUBLIC	void    hwint15();


/* proc.c */
PUBLIC	void 	schedule();
PUBLIC	void 	TestA();
PUBLIC 	void	TestB();
PUBLIC	void	TestC();


/* syscall.asm */
PUBLIC	void 	sys_call();
PUBLIC	int		get_ticks();
PUBLIC  void    write(char* buf, int len);
PUBLIC  int     sys_write(char* buf, int len, PROCESS* p_proc);



/* lib/memory.asm */
PUBLIC	void*	memcpy(void* p_dst, void* p_src, int size);
PUBLIC	void	memset(void* p_dst, char ch, int size);
PUBLIC  char*   strcpy(char* p_dst, char* p_src);
PUBLIC	int	    strlen(char* p_str);


/* lib/klib.c */
PUBLIC char * itoa(char * str, int num);
PUBLIC void disp_int(int input);


/* lib/kliba.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);
PUBLIC void disable_irq(int irq);
PUBLIC void enable_irq(int irq);
PUBLIC void disable_int();  /* open or close Interupt. */
PUBLIC void enable_int();