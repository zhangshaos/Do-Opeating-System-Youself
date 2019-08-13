/*================================================================================================
File name:		kernel/proc.c
Description:	*进程调度
				*系统调用定义
Copyright:		Chauncey Zhang
Date:		 	2019-7-14
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "tty.h"
#include "console.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "global.h"
#include "proto.h"
#include "global.h"
#include "stdio.h"
#include "log.h"

PRIVATE void block(PROCESS* p);
PRIVATE void unblock(PROCESS* p);
PRIVATE int  msg_send(PROCESS* current, int dest, MESSAGE* m);
PUBLIC  int  msg_receive(PROCESS* current, int src, MESSAGE* m);
PRIVATE int  deadlock(int src, int dest);

/*****************************************************************************
 *                                schedule
 *****************************************************************************/
/**
 * <Ring 0> Choose one proc to run.
 * 
 *****************************************************************************/
PUBLIC void schedule()
{
	PROCESS* p;
	int	 greatest_ticks = 0;

	while (!greatest_ticks) {
		for (p = &FIRST_PROC; p <= &LAST_PROC; p++) {
			if (p->p_flags == 0) {
				if (p->ticks > greatest_ticks) {
					greatest_ticks = p->ticks;
					p_proc_ready = p;
				}
			}
		}

		if (!greatest_ticks) {
			for (p = &FIRST_PROC; p <= &LAST_PROC; p++) {
				if (p->p_flags == 0) {
					p->ticks = p->priority;
				}
			}
		}
	}
}



/*****************************************************************************
 *                                sys_sendrec
 *****************************************************************************/
/**
 * <Ring 0> The core routine of system call `sendrec()'.
 * 
 * @param function SEND or RECEIVE
 * @param src_dest To/From whom the message is transferred.
 * @param m        Ptr to the MESSAGE body.
 * @param p        The caller proc.
 * 
 * @return Zero if success.
 *****************************************************************************/
PUBLIC int sys_sendrec(int function, int src_dest, MESSAGE* m, PROCESS* p)
{
	LOG_CALLS(p,"sys_sdcv");
	assert(k_reenter == 0);	/* make sure we are not in ring0 */
	assert((src_dest >= 0 && src_dest < NR_TASKS + NR_PROCS) ||
	       src_dest == ANY ||
	       src_dest == INTERRUPT);

	int ret = 0;
	int caller = proc2pid(p);
	MESSAGE* mla = (MESSAGE*)va2la(caller, m);
	mla->source = caller;

	assert(mla->source != src_dest);

	/**
	 * Actually we have the third message type: BOTH. However, it is not
	 * allowed to be passed to the kernel directly. Kernel doesn't know
	 * it at all. It is transformed into a SEND followed by a RECEIVE
	 * by `send_recv()'.
	 */
	if (function == SEND) {
		LOG_IPC(function,mla->source,src_dest,m);
		ret = msg_send(p, src_dest, m);
		if (ret != 0)
			return ret;
	}
	else if (function == RECEIVE) {
		LOG_IPC(function,mla->source,src_dest,m);
		ret = msg_receive(p, src_dest, m);
		if (ret != 0)
			return ret;
	}
	else {
		panic("{sys_sendrec} invalid function: "
		      "%d (SEND:%d, RECEIVE:%d).", function, SEND, RECEIVE);
	}

	LOG_RETS(p,"sys_sdrv");
	return 0;
}

/*****************************************************************************
 *                                send_recv
 *****************************************************************************/
/**
 * <Ring 1~3> IPC syscall.
 *
 * It is an encapsulation of `sendrec',
 * invoking `sendrec' directly should be avoided
 *
 * @param function  SEND, RECEIVE or BOTH
 * @param src_dest  The caller's proc_nr
 * @param msg       Pointer to the MESSAGE struct
 * 
 * @return always 0.
 *****************************************************************************/
PUBLIC int send_recv(int function, int src_dest, MESSAGE* msg)
{
	int ret = 0;

	if (function == RECEIVE)
		memset(msg, 0, sizeof(MESSAGE));

	switch (function) 
	{
	case BOTH:
		ret = sendrec(SEND, src_dest, msg);
		if (ret == 0)
			ret = sendrec(RECEIVE, src_dest, msg);
		break;
	case SEND:
	case RECEIVE:
		ret = sendrec(function, src_dest, msg);
		break;
	default:
		assert((function == BOTH) ||
		       (function == SEND) || (function == RECEIVE));
		break;
	}

	return ret;
}

/*****************************************************************************
 *				  ldt_seg_linear
 *****************************************************************************/
/**
 * <Ring 0~1> Calculate the linear address of a certain segment of a given
 * proc.
 * 
 * @param p   Whose (the proc ptr).
 * @param idx Which (one proc has more than one segments).
 * 
 * @return  The required linear address.
 *****************************************************************************/
PUBLIC int ldt_seg_linear(PROCESS* p, int idx)
{
	DESCRIPTOR * d = p->ldts + idx;
	return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}

/*****************************************************************************
 *				  va2la
 *****************************************************************************/
/**
 * <Ring 0~1> Virtual addr --> Linear addr.
 * 
 * @param pid  PID of the proc whose address is to be calculated.
 * @param va   Virtual address.
 * 
 * @return The linear address for the given virtual address.
 *****************************************************************************/
PUBLIC void* va2la(int pid, void* va)
{
	PROCESS* p = &proc_table[pid];

	u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
	u32 la = seg_base + (u32)va;

	if (pid < NR_TASKS + NR_NATIVE_PROCS) {
		assert(la == (u32)va);
	}

	// if(la != (u32)va)
	// {
	// 	printl(" va:%x,la:%x ",(u32)va,la);
	// }
	// emmm, printl->sys_print->va2la : 无限递归...
	// if(la!=(u32)va)
	// 	LOG_RECORD("va:%x,la:%x",(u32)va,la);
	
	return (void*)la;
}

/*****************************************************************************
 *                                reset_msg
 *****************************************************************************/
/**
 * <Ring 0~3> Clear up a MESSAGE by setting each byte to 0.
 * 
 * @param p  The message to be cleared.
 *****************************************************************************/
PUBLIC void reset_msg(MESSAGE* p)
{
	memset(p, 0, sizeof(MESSAGE));
}

/*****************************************************************************
 *                                block
 *****************************************************************************/
/**
 * <Ring 0> This routine is called after `p_flags' has been set (!= 0), it
 * calls `schedule()' to choose another proc as the `proc_ready'.
 *
 * @attention This routine does not change `p_flags'. Make sure the `p_flags'
 * of the proc to be blocked has been set properly.
 * 
 * @param p The proc to be blocked.
 *****************************************************************************/
PRIVATE void block(PROCESS* p)
{
	assert(p->p_flags);
	schedule();
}

/*****************************************************************************
 *                                unblock
 *****************************************************************************/
/**
 * <Ring 0> This is a dummy routine. It does nothing actually. When it is
 * called, the `p_flags' should have been cleared (== 0).
 * 
 * @param p The unblocked proc.
 *****************************************************************************/
PRIVATE void unblock(PROCESS* p)
{
	assert(p->p_flags == 0);
}

/*****************************************************************************
 *                                deadlock
 *****************************************************************************/
/**
 * <Ring 0> Check whether it is safe to send a message from src to dest.
 * The routine will detect if the messaging graph contains a cycle. For
 * instance, if we have procs trying to send messages like this:
 * A -> B -> C -> A, then a deadlock occurs, because all of them will
 * wait forever. If no cycles detected, it is considered as safe.
 * 
 * @param src   Who wants to send message.
 * @param dest  To whom the message is sent.
 * 
 * @return Zero if success.
 *****************************************************************************/
PRIVATE int deadlock(int src, int dest)
{
	PROCESS* p = proc_table + dest;
	while (1) {
		if (p->p_flags & SENDING) {
			if (p->p_sendto == src) {
				/* print the chain */
				p = proc_table + dest;
				printl("=_=%s", p->name);
				do {
					assert(p->p_hold_msg);
					p = proc_table + p->p_sendto;
					printl("->%s", p->name);
				} while (p != proc_table + src);
				printl("=_=");

				return 1;
			}
			p = proc_table + p->p_sendto;
		}
		else {
			break;
		}
	}
	return 0;
}

/*****************************************************************************
 *                                msg_send
 *****************************************************************************/
/**
 * <Ring 0> Send a message to the dest proc. If dest is blocked waiting for
 * the message, copy the message to it and unblock dest. Otherwise the caller
 * will be blocked and appended to the dest's sending queue.
 * 
 * @param current  The caller, the sender.
 * @param dest     To whom the message is sent.
 * @param m        The message.
 * 
 * @return Zero if success.
 *****************************************************************************/
PRIVATE int msg_send(PROCESS* current, int dest, MESSAGE* m)
{
	PROCESS* sender = current;
	PROCESS* p_dest = proc_table + dest; /* proc dest */

	assert(proc2pid(sender) != dest);

	/* check for deadlock here */
	if (deadlock(proc2pid(sender), dest)) 
	{
		panic(">>DEADLOCK<< %s->%s", sender->name, p_dest->name);
	}

	if ((p_dest->p_flags & RECEIVING) && /* dest is waiting for the msg */
	    (p_dest->p_recvfrom == proc2pid(sender) || p_dest->p_recvfrom == ANY)) 
	{
		assert(p_dest->p_hold_msg);
		assert(m);

		/**
		 * 将this(sender)的消息写入p_dest
		 * 保持(hold)的receiving消息
		 */
		phys_copy(va2la(dest, p_dest->p_hold_msg),
			  va2la(proc2pid(sender), m),
			  sizeof(MESSAGE));

		p_dest->p_hold_msg = 0;
		p_dest->p_flags &= ~RECEIVING; 	/* dest has received the msg */
		p_dest->p_recvfrom = NO_TASK;
		unblock(p_dest);

		/**@ assert()
		 * @sender:
		 * p_dest->p_flags, p_hold_msg = 0
		 * 		 ->p_recvfrom = NO_TASK (Because you are ready for send not for receive)
		 * 		 ->p_sendto = NO_TASK (Because you have sent just now...)
		 * 
		 * @p_dest:
		 * p_dest->p_flags = 0 (Because you have unlocked it)
		 * 		 ->p_hold_msg = 0 (Because you have give it the receiving msg that it should hold)
		 * 		 ->p_recvfrom = NO_TASK (Because you have sent just now...)
		 * 		 ->p_sendto = NO_TASK (Because it had been receiving status(it's impossible for sending) before you sent just now.)
		 */
		assert(p_dest->p_flags == 0);
		assert(p_dest->p_hold_msg == 0);
		assert(p_dest->p_recvfrom == NO_TASK);
		assert(p_dest->p_sendto == NO_TASK);
		assert(sender->p_flags == 0);
		assert(sender->p_hold_msg == 0);
		assert(sender->p_recvfrom == NO_TASK);
		assert(sender->p_sendto == NO_TASK);
	}
	else { /* dest is not waiting for the msg */
		sender->p_flags |= SENDING;
		assert(sender->p_flags == SENDING);
		sender->p_sendto = dest;
		sender->p_hold_msg = m;

		/* append to the sending queue */
		PROCESS * p;
		if (p_dest->q_sending) 
		{
			p = p_dest->q_sending;
			while (p->next_sending)
			{
				p = p->next_sending;
			}
			p->next_sending = sender;
		}
		else {
			p_dest->q_sending = sender;
		}
		sender->next_sending = 0;

		block(sender);

		assert(sender->p_flags == SENDING);
		assert(sender->p_hold_msg != 0);	/* Because sender is sending status and hold the sending msg. */
		assert(sender->p_recvfrom == NO_TASK);
		assert(sender->p_sendto == dest);
	}

	return 0;
}


/*****************************************************************************
 *                                msg_receive
 *****************************************************************************/
/**
 * <Ring 0> Try to get a message from the src proc. If src is blocked sending
 * the message, copy the message from it and unblock src. Otherwise the caller
 * will be blocked.
 * 
 * @param current The caller, the proc who wanna receive.
 * @param src     From whom the message will be received.
 * @param m       The message ptr to accept the message.
 * 
 * @return  Zero if success.
 *****************************************************************************/
PUBLIC int msg_receive(PROCESS* current, int src, MESSAGE* m)
{
	PROCESS* p_who_wanna_recv = current; /**
						  * This name is a little bit
						  * wierd, but it makes me
						  * think clearly, so I keep
						  * it.
						  */
	PROCESS* p_from = 0; /* from which the message will be fetched */
	PROCESS* prev = 0;
	int deliver_msg_ready = 0;

	assert(proc2pid(p_who_wanna_recv) != src);

	if ((p_who_wanna_recv->has_int_msg) &&
	    ((src == ANY) || (src == INTERRUPT))) //如果receiver 有一个硬件中断需要处理
	{
		/* There is an interrupt needs p_who_wanna_recv's handling and
		 * p_who_wanna_recv is ready to handle it.
		 */

		/** 如果this有一个中断来了,
		 * 立即创建一个INTERRUPT的msg,
		 * 并将该消息直接发送给this(receiver)
		 */
		MESSAGE msg;
		reset_msg(&msg);
		msg.source = INTERRUPT;
		msg.type = HARD_INT;
		assert(m);
		/**
		 * 将此消息直接传递给this(receiver)->准备好的msg
		 * Q: 为什么不传递给p_hold_msg?
		 * A: 因为此时this还没有阻塞在RECEIVING状态,并没有hold任何消息.
		 */
		phys_copy(va2la(proc2pid(p_who_wanna_recv), m),
					&msg,
			  		sizeof(MESSAGE));

		p_who_wanna_recv->has_int_msg = 0;

		assert(p_who_wanna_recv->p_flags == 0);
		assert(p_who_wanna_recv->p_hold_msg == 0);
		assert(p_who_wanna_recv->p_sendto == NO_TASK);
		assert(p_who_wanna_recv->has_int_msg == 0);

		return 0;
	}


	/* Arrives here if no interrupt for p_who_wanna_recv. */
	if (src == ANY) //如果receiver 在忙等
	{
		/* p_who_wanna_recv is ready to receive messages from
		 * ANY proc, we'll check the sending queue and pick the
		 * first proc in it.
		 */
		if (p_who_wanna_recv->q_sending) 
		{
			p_from = p_who_wanna_recv->q_sending;
			/* ready for p_from->hold_msg -> m */
			deliver_msg_ready = 1;

			assert(p_who_wanna_recv->p_flags == 0);
			assert(p_who_wanna_recv->p_hold_msg == 0);
			assert(p_who_wanna_recv->p_recvfrom == NO_TASK);
			assert(p_who_wanna_recv->p_sendto == NO_TASK);
			assert(p_who_wanna_recv->q_sending != 0);
			assert(p_from->p_flags == SENDING);
			assert(p_from->p_hold_msg != 0);
			assert(p_from->p_recvfrom == NO_TASK);
			assert(p_from->p_sendto == proc2pid(p_who_wanna_recv));
		}
	}
	else if(src >= 0 && src < NR_TASKS+NR_NATIVE_PROCS+NR_PROCS) //如果receiver 在等某一个具体的进程消息
	{
		p_from = proc_table + src;

		if ((p_from->p_flags & SENDING) &&
		    (p_from->p_sendto == proc2pid(p_who_wanna_recv))) 
		{
			/* Perfect, src is sending a message to
			 * p_who_wanna_recv.
			 */

			/* ready for p_from->hold_msg -> m */
			deliver_msg_ready = 1;

			PROCESS* p = p_who_wanna_recv->q_sending;
			assert(p); /* p_from must have been appended to the
				    * queue, so the queue must not be NULL
				    */
			while (p) 
			{
				assert(p_from->p_flags & SENDING);
				if (proc2pid(p) == src) //if src is the first on the q_seeding queue.
				{
					p_from = p;
					break;
				}
				prev = p;
				p = p->next_sending;
			}

			assert(p_who_wanna_recv->p_flags == 0);
			assert(p_who_wanna_recv->p_hold_msg == 0);
			assert(p_who_wanna_recv->p_recvfrom == NO_TASK);
			assert(p_who_wanna_recv->p_sendto == NO_TASK);
			assert(p_who_wanna_recv->q_sending != 0);
			assert(p_from->p_flags == SENDING);
			assert(p_from->p_hold_msg != 0);
			assert(p_from->p_recvfrom == NO_TASK);
			assert(p_from->p_sendto == proc2pid(p_who_wanna_recv));
		}
	}
	else // src == INTERRUPT, receiver 在等待中断
	{
		assert(src == INTERRUPT);
		assert(p_who_wanna_recv->p_flags == 0);

		p_who_wanna_recv->p_flags 		|=	RECEIVING;
		p_who_wanna_recv->p_hold_msg 	=	m;
		p_who_wanna_recv->p_recvfrom	=	src;

		block(p_who_wanna_recv);
		// 这里设计得还是有问题,因为硬件中断可能随时发生
		// 如果在block()调用前,突然硬件中断修改了p_who_wanna_recv->p_flags,会导致系统直接assert失败而hlt
		// 必须要想一个办法, 解决边界问题......
		// ......

		return 0;
	}


	if (deliver_msg_ready) 
	{
		/* It's determined from which proc the message will
		 * be copied. Note that this proc must have been
		 * waiting for this moment in the queue, so we should
		 * remove it from the queue.
		 */
		if (p_from == p_who_wanna_recv->q_sending) /* the 1st one */
		{
			assert(prev == 0);
			p_who_wanna_recv->q_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}
		else 
		{
			assert(prev);
			prev->next_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}

		assert(m);
		assert(p_from->p_hold_msg);
		/* copy the message */
		phys_copy(va2la(proc2pid(p_who_wanna_recv), m),
					va2la(proc2pid(p_from), p_from->p_hold_msg),
			  		sizeof(MESSAGE));

		p_from->p_hold_msg 	= 0;
		p_from->p_sendto 	= NO_TASK;
		p_from->p_flags 	&= ~SENDING;
		unblock(p_from);
	}
	else 
	{
		/**
		 * @ WARNING
		 * 这部分IPC设计有问题,如果进程等待的是硬件中断
		 * 那么必须要考虑中断可能时刻发生!
		 * 所以必须针对INTERRUPT重新设计IPC
		 * (我觉得把中断和IPC分开设计要好得多...
		 * Linux说得没错,IPC看起来非常完美,实际上实现完美的IPC非常困难)
		 */

		/**
		 * nobody's sending any msg.
		 * so, make this process in RECEIVING status 
		 * and lock it.
		 */

		p_who_wanna_recv->p_flags |= RECEIVING;

		p_who_wanna_recv->p_hold_msg = m;

		p_who_wanna_recv->p_recvfrom = src;

		block(p_who_wanna_recv);

		/**
		 * what about coming here with interrupt occuring,
		 * then, p_flags will change
		 */
		assert(p_who_wanna_recv->p_flags == RECEIVING);
		assert(p_who_wanna_recv->p_hold_msg != 0);
		assert(p_who_wanna_recv->p_recvfrom != NO_TASK);
		assert(p_who_wanna_recv->p_sendto == NO_TASK);
		//printl("receiver:%d, msg_from:%d\n",current-proc_table,src);
		/**@ Amazing!
		 * when cotrol flow run in printf(), a HD interrupt happen
		 * and set has_int_msg = 1 !
		 * But, if control flow come to HD INT handler and invoke inform_int(),
		 * why inform_int won't giver msg to hd_task() and set has_int_msg = 0 directly?
		 * A : I got it!
		 * Because p_dest->p_recvform == TASK_SYS...
		 * 
		 * 
		 * @ Further thought
		 * If a process don't wait for a INT but another INT,
		 * the former can give its msg to the process compulsorily ?
		 * Obviously not ! 
		 * 
		 * @ Final,
		 * So I think this assert isn't necessary !
		 */
		// assert(p_who_wanna_recv->has_int_msg == 0);
	}

	return 0;
}


/*****************************************************************************
 *                                inform_int
 *****************************************************************************/
/**
 * <Ring 0> Inform a proc that an interrupt has occured.
 * 
 * @param task_nr  The task which will be informed.
 *****************************************************************************/
PUBLIC void inform_int(int task_nr)
{
	PROCESS *p_dest = proc_table + task_nr;

	if ((p_dest->p_flags & RECEIVING) && /* dest is waiting for the msg */
	    ((p_dest->p_recvfrom == INTERRUPT) || (p_dest->p_recvfrom == ANY))) 
	{
		p_dest->p_hold_msg->source 	= INTERRUPT;
		p_dest->p_hold_msg->type 	= HARD_INT;

		/**Q: p_hold_msg为什么要清零?
		 * A: 因为p_dest已经"收到了"想要的msg, 接下来unlock(p_dest),
		 * 	  p_dest就会正式接收处理这个msg.
		 * 	  所以, p_dest不在处于RECEIVING状态而保持(hold)msg了.
		 */
		p_dest->p_hold_msg 			= 0;
		p_dest->has_int_msg 		= 0;
		p_dest->p_flags 			&= ~RECEIVING; /* dest has received the msg */
		p_dest->p_recvfrom 			= NO_TASK;

		assert(p_dest->p_flags == 0);
		
		unblock(p_dest);

		assert(p_dest->p_flags == 0);
		assert(p_dest->p_hold_msg == 0);
		assert(p_dest->p_recvfrom == NO_TASK);
		assert(p_dest->p_sendto == NO_TASK);
	}
	else
	{
		p_dest->has_int_msg = 1;
		/* 当 p_dest 调用msg_receive()时,会优先检查自己是否有硬件消息(has_int_msg) */
	}
}


/*****************************************************************************
 *                                dump_proc
 *****************************************************************************/
PUBLIC void dump_proc(PROCESS* p)
{
	char info[STR_DEFAULT_LEN];
	int i;
	int text_color = MAKE_COLOR(GREEN, RED);

	int dump_len = sizeof(PROCESS);

	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, 0);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, 0);

	sprintf(info, "byte dump of proc_table[%d]:\n", p - proc_table); disp_color_str(info, text_color);
	for (i = 0; i < dump_len; i++) {
		sprintf(info, "%x.", ((unsigned char *)p)[i]);
		disp_color_str(info, text_color);
	}

	/* printl("^^"); */

	disp_color_str("\n\n", text_color);
	sprintf(info, "ANY: 0x%x.\n", ANY); disp_color_str(info, text_color);
	sprintf(info, "NO_TASK: 0x%x.\n", NO_TASK); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);

	sprintf(info, "ldt_sel: 0x%x.  ", p->ldt_sel); disp_color_str(info, text_color);
	sprintf(info, "ticks: 0x%x.  ", p->ticks); disp_color_str(info, text_color);
	sprintf(info, "priority: 0x%x.  ", p->priority); disp_color_str(info, text_color);
	// sprintf(info, "pid: 0x%x.  ", p->pid); disp_color_str(info, text_color);
	sprintf(info, "name: %s.  ", p->name); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);
	sprintf(info, "p_flags: 0x%x.  ", p->p_flags); disp_color_str(info, text_color);
	sprintf(info, "p_recvfrom: 0x%x.  ", p->p_recvfrom); disp_color_str(info, text_color);
	sprintf(info, "p_sendto: 0x%x.  ", p->p_sendto); disp_color_str(info, text_color);
	// sprintf(info, "nr_tty: 0x%x.  ", p->nr_tty); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);
	sprintf(info, "has_int_msg: 0x%x.  ", p->has_int_msg); disp_color_str(info, text_color);
}


/*****************************************************************************
 *                                dump_msg
 *****************************************************************************/
PUBLIC void dump_msg(const char * title, MESSAGE* m)
{
	int packed = 0;
	printl("{%s}<0x%x>{%ssrc:%s(%d),%stype:%d,%s(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)%s}%s",  //, (0x%x, 0x%x, 0x%x)}",
	       title,
	       (int)m,
	       packed ? "" : "\n        ",
	       proc_table[m->source].name,
	       m->source,
	       packed ? " " : "\n        ",
	       m->type,
	       packed ? " " : "\n        ",
	       m->u.m3.m3i1,
	       m->u.m3.m3i2,
	       m->u.m3.m3i3,
	       m->u.m3.m3i4,
	       (int)m->u.m3.m3p1,
	       (int)m->u.m3.m3p2,
	       packed ? "" : "\n",
	       packed ? "" : "\n"/* , */
		);
}
