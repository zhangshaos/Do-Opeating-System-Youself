/*================================================================================================
File name:		kernel/keyboard.c
Description:	*
Copyright:		Chauncey Zhang
Date:		 	2019-7-18
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "keymap.h"
#include "proto.h"



PRIVATE	int	code_with_E0;
PRIVATE	int	shift_l;	/* l shift state */
PRIVATE	int	shift_r;	/* r shift state */
PRIVATE	int	alt_l;		/* l alt state	 */
PRIVATE	int	alt_r;		/* r left state	 */
PRIVATE	int	ctrl_l;		/* l ctrl state	 */
PRIVATE	int	ctrl_r;		/* l ctrl state	 */
PRIVATE	int	caps_lock;	/* Caps Lock	 */
PRIVATE	int	num_lock;	/* Num Lock	 */
PRIVATE	int	scroll_lock;	/* Scroll Lock	 */
PRIVATE	int	column;



PRIVATE u8		get_byte_from_kbuf();
PRIVATE void    set_leds();
PRIVATE void    kb_wait();
PRIVATE void    kb_ack();

/*======================================================================*
                            keyboard_handler
	=================================================================
				键盘中断处理程序,将扫描嘛写入扫描码缓冲区中
 *======================================================================*/
PUBLIC void keyboard_handler(int irq)
{
	u8 scan_code = in_byte(KB_DATA);

	if (KB_CODE_INBUF.count < KB_IN_BYTES) {
		*(KB_CODE_INBUF.p_head) = scan_code;
		KB_CODE_INBUF.p_head++;
		if (KB_CODE_INBUF.p_head == KB_CODE_INBUF.buf + KB_IN_BYTES) {
			KB_CODE_INBUF.p_head = KB_CODE_INBUF.buf;
		}
		KB_CODE_INBUF.count++;
	}

	key_pressed = 1;
}


/*======================================================================*
                           init_keyboard
	=================================================================
							允许接受键盘中断
*======================================================================*/
PUBLIC void init_keyboard()
{
	KB_CODE_INBUF.count = 0;
	KB_CODE_INBUF.p_head = KB_CODE_INBUF.p_tail = KB_CODE_INBUF.buf;

	shift_l	= shift_r = 0;
	alt_l	= alt_r   = 0;
	ctrl_l	= ctrl_r  = 0;

	caps_lock   = 0;
	num_lock    = 1;
	scroll_lock = 0;

	column		= 0;

	set_leds();

        put_irq_handler(KEYBOARD_IRQ, keyboard_handler);/*设定键盘中断处理程序*/
        enable_irq(KEYBOARD_IRQ);                       /*开键盘中断*/
}


/*======================================================================*
                           keyboard_read
	从扫描码缓冲区中解析扫描码, 将其转化位自定义可识别的键key(包含功能码和字符)
	最后, 调用in_process():
						完成功能码的功能 && 将字符写入对应tty的缓冲中
*======================================================================*/
PUBLIC void keyboard_read(TTY* p_tty)
{
	u8	scan_code;
	int	make;	/* 1: make;  0: break. */

	u32	key = 0;/* 用一个整型来表示一个键。比如，如果 Home 被按下，
			 * 则 key 值将为定义在 keyboard.h 中的 'HOME'。
			 */
	u32*	keyrow;	/* 指向 keymap[] 的某一行 */

	if(KB_CODE_INBUF.count > 0)
	{
		code_with_E0 = 0;

		scan_code = get_byte_from_kbuf();	/* 获取扫描码, 没有扫描码时忙等 */

		/* 下面开始解析扫描码,
		先判断两个扫描码-make长度>2的特殊按键 */
		if (scan_code == 0xE1) 
		{
			int i;
			u8 pausebrk_scode[] = {0xE1, 0x1D, 0x45, 0xE1, 0x9D, 0xC5};

			int is_pausebreak = 1;
			/* 测试按键是不是PauseBreak */
			for(i=1;i<6;i++)
			{
				if (get_byte_from_kbuf() != pausebrk_scode[i]) 
				{
					is_pausebreak = 0;
					break;
				}
			}
			if (is_pausebreak) 
			{
				key = PAUSEBREAK;
			}
		}
		else if (scan_code == 0xE0) 
		{
			scan_code = get_byte_from_kbuf();

			/* 测试PrintScreen 被按下 */
			if (scan_code == 0x2A) {
				if (get_byte_from_kbuf() == 0xE0) {
					if (get_byte_from_kbuf() == 0x37) {
						key = PRINTSCREEN;
						make = 1;
					}
				}
			}
			/* 测试PrintScreen 被释放 */
			if (scan_code == 0xB7) {
				if (get_byte_from_kbuf() == 0xE0) {
					if (get_byte_from_kbuf() == 0xAA) {
						key = PRINTSCREEN;
						make = 0;
					}
				}
			}
			/* 不是PrintScreen, 此时scan_code为0xE0紧跟的那个值. */
			if (key == 0) 
			{
				code_with_E0 = 1;
			}
		}


		/* 如果是一般按键 */
		if ((key != PAUSEBREAK) && (key != PRINTSCREEN)) 
		{
			/* 首先判断Make Code 还是 Break Code */
			make = (scan_code & FLAG_BREAK ? 0 : 1);

			/* 先定位到 keymap 中的行 */
			keyrow = &keymap[(scan_code & 0x7F) * MAP_COLS];

			column = 0;

			int caps = shift_l || shift_r;
			if (caps_lock)
			{
				/* 如果不是按键"字符", 那么大写锁定没什么用 */
				if ((keyrow[0] >= 'a') && (keyrow[0] <= 'z')){
					caps = !caps;
				}
			}
			if (caps) {
				column = 1;
			}

			if (code_with_E0) {
				column = 2;
			}

			key = keyrow[column];

			switch(key) {
			case SHIFT_L:
				shift_l = make;
				break;
			case SHIFT_R:
				shift_r = make;
				break;
			case CTRL_L:
				ctrl_l = make;
				break;
			case CTRL_R:
				ctrl_r = make;
				break;
			case ALT_L:
				alt_l = make;
				break;
			case ALT_R:
				alt_l = make;
				break;
			case CAPS_LOCK:
				if (make) {
					caps_lock   = !caps_lock;
					set_leds();
				}
				break;
			case NUM_LOCK:
				if (make) {
					num_lock    = !num_lock;
					set_leds();
				}
				break;
			case SCROLL_LOCK:
				if (make) {
					scroll_lock = !scroll_lock;
					set_leds();
				}
				break;
			default:
				break;
			}


			/* 忽略 Break Code, 只处理 Make Code */
			if (make) 
			{
				int pad = 0;

				/* 首先处理小键盘 */
				if ((key >= PAD_SLASH) && (key <= PAD_9))
				{
					pad = 1;
					switch(key) {
					case PAD_SLASH:
						key = '/';
						break;
					case PAD_STAR:
						key = '*';
						break;
					case PAD_MINUS:
						key = '-';
						break;
					case PAD_PLUS:
						key = '+';
						break;
					case PAD_ENTER:
						key = ENTER;
						break;
					default:
						if (num_lock &&
						    (key >= PAD_0) &&
						    (key <= PAD_9)) 
						{
							key = key - PAD_0 + '0';
						}
						else if (num_lock &&
							 (key == PAD_DOT)) 
						{
							key = '.';
						}
						else
						{
							switch(key) 
							{
							case PAD_HOME:
								key = HOME;
								break;
							case PAD_END:
								key = END;
								break;
							case PAD_PAGEUP:
								key = PAGEUP;
								break;
							case PAD_PAGEDOWN:
								key = PAGEDOWN;
								break;
							case PAD_INS:
								key = INSERT;
								break;
							case PAD_UP:
								key = UP;
								break;
							case PAD_DOWN:
								key = DOWN;
								break;
							case PAD_LEFT:
								key = LEFT;
								break;
							case PAD_RIGHT:
								key = RIGHT;
								break;
							case PAD_DOT:
								key = DELETE;
								break;
							default:
								break;
							}
						}
						break;
					}
				}

				key |= shift_l	? FLAG_SHIFT_L	: 0;
				key |= shift_r	? FLAG_SHIFT_R	: 0;
				key |= ctrl_l	? FLAG_CTRL_L	: 0;
				key |= ctrl_r	? FLAG_CTRL_R	: 0;
				key |= alt_l	? FLAG_ALT_L	: 0;
				key |= alt_r	? FLAG_ALT_R	: 0;
				key |= pad      ? FLAG_PAD      : 0;

				in_process(p_tty, key);
			}
		}
	}
}

/*======================================================================*
			    get_byte_from_kbuf
				从键盘扫描码缓冲中,取一个扫描码
 *======================================================================*/
PRIVATE u8 get_byte_from_kbuf()
{
        u8 scan_code;

        while (KB_CODE_INBUF.count <= 0) {}   /* 等待下一个字节到来 */

        disable_int();	/* forbid interrupt */
        scan_code = *(KB_CODE_INBUF.p_tail);
        KB_CODE_INBUF.p_tail++;
        if (KB_CODE_INBUF.p_tail == KB_CODE_INBUF.buf + KB_IN_BYTES) 
		{
                KB_CODE_INBUF.p_tail = KB_CODE_INBUF.buf;
        }
        KB_CODE_INBUF.count--;
        enable_int();	/* resume interrupt */

	return scan_code;
}

/*======================================================================*
				 kb_wait
 *======================================================================*/
PRIVATE void kb_wait()	/* 等待 8042 的输入缓冲区空 */
{
	u8 kb_stat;

	do {
		kb_stat = in_byte(KB_CMD);
	} while (kb_stat & 0x02);
}


/*======================================================================*
				 kb_ack
 *======================================================================*/
PRIVATE void kb_ack()	/* 等待键盘回复 */
{
	u8 kb_read;

	do {
		kb_read = in_byte(KB_DATA);
	} while (kb_read != KB_ACK);
}

/*======================================================================*
				 set_leds
 *======================================================================*/
PRIVATE void set_leds()
{
	u8 leds = (caps_lock << 2) | (num_lock << 1) | scroll_lock;

	kb_wait();
	out_byte(KB_DATA, LED_CODE);	/* 准备设置键盘灯 */
	kb_ack();

	kb_wait();
	out_byte(KB_DATA, leds);	/* 设置键盘灯 */
	kb_ack();
}

