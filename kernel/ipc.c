




#include "type.h"
#include "const.h"
#include "struct_proc.h"
#include "global.h"
#include "func_proto.h"



PRIVATE void block(PROCESS* p);
PRIVATE void unblock(PROCESS* p);
PRIVATE int  msg_send(PROCESS* current, int dest, MESSAGE* m);
PRIVATE int  msg_receive(PROCESS* current, int src, MESSAGE* m);
PRIVATE int  deadlock(int src, int dest);



/** ================= sys_sendrec =======================
 * <Ring 0> The core routine of system call `sendrec()'.
 * 
 * @param function SEND or RECEIVE
 * @param src_dest To/From whom the message is transferred.
 * @param m        Ptr to the MESSAGE body.
 * @param p        The caller proc.
 * 
 * @return Zero if success.
 */
 PUBLIC int sys_sendrec(int function, int src_dest, MESSAGE* m, PROCESS* p)
{
	assert(k_reenter == 0); /* make sure we are in ring0 now. */
	assert((src_dest >= 0 && src_dest < NR_TASKS + NR_PROCS) ||
	       src_dest == ANY ||
	       src_dest == INTERRUPT);

    /* address convert */
    /**
     * In Ring1-3 code, the address is virtual,
     * so, you must convert it to physical in Ring0.
     */
	int id_caller = proc2pid(p);
	MESSAGE* mla = (MESSAGE*)va2la(id_caller, m);
	mla->source = id_caller;

 	assert(mla->source != src_dest);

 	/**
	 * Actually we have the third message type: BOTH. However, it is not
	 * allowed to be passed to the kernel directly. Kernel doesn't know
	 * it at all. It is transformed into a SEND followed by a RECEIVE
	 * by `send_recv()'.
	 */
 	int ret = 0;
	if (function == SEND) 
    {
		ret = msg_send(p, src_dest, m);
		if (ret != 0)
			return ret;
	}
	else if (function == RECEIVE) 
    {
		ret = msg_receive(p, src_dest, m);
		if (ret != 0)
			return ret;
	}
	else {
		panic("{sys_sendrec} invalid function: "
		      "%d (SEND:%d, RECEIVE:%d).", function, SEND, RECEIVE);
	}

 	return 0;
}




/** ===================== send_recv ======================
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
 */
PUBLIC int send_recv(int function, int src_dest, MESSAGE* msg)
{
 	if (function == RECEIVE)
		memset(msg, 0, sizeof(MESSAGE));

	int ret = 0;
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


/** ==================== reset_msg =====================
 * <Ring 0~3> Clear up a MESSAGE by setting each byte to 0.
 * 
 * @param p  The message to be cleared.
 */
PUBLIC void reset_msg(MESSAGE* p)
{
	memset(p, 0, sizeof(MESSAGE));
}



/** ============================= msg_send ================================
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

 	if ((p_dest->p_status & RECEIVING) && /* dest is waiting for the msg */
	    (p_dest->p_want_recvfrom == proc2pid(sender) ||
	     p_dest->p_want_recvfrom == ANY)) 
    {
		assert(p_dest->p_hold_msg);
		assert(m);

        /**
		 * 将this(sender)的消息写入p_dest
		 * 保持(hold)的receiving消息
		 */
 		memcpy(va2la(dest, p_dest->p_hold_msg),
			  va2la(proc2pid(sender), m),
			  sizeof(MESSAGE));
		p_dest->p_hold_msg = 0;
		p_dest->p_status &= ~RECEIVING; /* dest has received the msg */
		p_dest->p_want_recvfrom = NO_TASK;
		unblock(p_dest); /* you have to set p_falgs before invoking unlock() */

		/**@ assert()
		 * @sender:
		 * p_dest->p_status, p_hold_msg = 0
		 * 		 ->p_want_recvfrom = NO_TASK (Because you are ready for send not for receive)
		 * 		 ->p_want_sendto = NO_TASK (Because you have sent just now...)
		 * 
		 * @p_dest:
		 * p_dest->p_status = 0 (Because you have unlocked it)
		 * 		 ->p_hold_msg = 0 (Because you have give it the receiving msg that it should hold)
		 * 		 ->p_want_recvfrom = NO_TASK (Because you have sent just now...)
		 * 		 ->p_want_sendto = NO_TASK (Because it had been receiving status(it's impossible for sending) before you sent just now.)
		 */
		assert(p_dest->p_status == 0);
		assert(p_dest->p_hold_msg == 0);
		assert(p_dest->p_want_recvfrom == NO_TASK);
		assert(p_dest->p_want_sendto == NO_TASK);    /* why ? A:初始化时设定*/
        /* why ? */
        /**
         * sender在准备发送msg时,
         * p_status = 0, 因为此时正在running;
         * p_hold_msg = 0, 因为此时还不是sending状态,还没有挂载msg(只是将msg准备好发送)
         * p_recvform 和 p_want_sendto = NO_TASK, 因为初始化时候设定
         */
		assert(sender->p_status == 0);
		assert(sender->p_hold_msg == 0);
		assert(sender->p_want_recvfrom == NO_TASK);
		assert(sender->p_want_sendto == NO_TASK);
	}
	else 
    {   /**
         * destination is not waitting for msg.
         * make caller blocked and sending, which 
         * is prepared for receiver.
         */
		sender->p_status |= SENDING;
		assert(sender->p_status == SENDING); /* 这个是真的多余... */
		sender->p_want_sendto = dest;
		sender->p_hold_msg = m;

 		/* append to the sending queue */
		PROCESS * p;
		if (p_dest->q_sending) 
        {
			p = p_dest->q_sending;
            /* 将sender挂载到最后一个向目的process发送消息的process->next_sending上 */
			while (p->next_sending)
				p = p->next_sending;
			p->next_sending = sender;
		}
		else 
        {
			p_dest->q_sending = sender;
		}
		sender->next_sending = 0;

        /* After modifying p_status, block this process. */
 		block(sender);

        /* unnecessary... */
 		assert(sender->p_status == SENDING);
		assert(sender->p_hold_msg != 0);
		assert(sender->p_want_recvfrom == NO_TASK);
		assert(sender->p_want_sendto == dest);
	}

 	return 0;
}


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
 */
PRIVATE int msg_receive(PROCESS* current, int src, MESSAGE* m)
{
	PROCESS* receiver = current; /**
						  * This name is a little bit
						  * wierd, but it makes me
						  * think clearly, so I keep
						  * it.
						  */

 	assert(proc2pid(receiver) != src);

    /**
     * 为什么不在其他地方处理进程的硬件中断消息?
     * 因为接收硬件中断消息, 这件事本来就应该是receiver的事情呀...
     * 中断是直接推送到CPU的, 没办法用IPC, 所以在msg_send()中不作检查.
     * (has_int_msg是由系统设定的, 不是IPC设定)
     * */
 	if ((receiver->has_int_msg) &&
	    ((src == ANY) || (src == INTERRUPT)))
    {
        /* There is an interrupt needs receiver's handling and
		 * receiver is ready to handle it.
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
		memcpy(va2la(proc2pid(receiver), m), &msg,
			  sizeof(MESSAGE));

 		receiver->has_int_msg = 0;

 		assert(receiver->p_status == 0);
		assert(receiver->p_hold_msg == 0);
		assert(receiver->p_want_sendto == NO_TASK);
		assert(receiver->has_int_msg == 0);

        /* 硬件消息,立即返回 */
 		return 0;
	}


	PROCESS* p_from = 0; /* from which the message will be fetched */
	PROCESS* prev = 0;
	int recv_ok = 0;

 	/* Arrives here if no interrupt for receiver. */
	if (src == ANY) 
    {
		/* receiver is ready to receive messages from
		 * ANY proc, we'll check the sending queue and pick the
		 * first proc in it.
		 */
		if (receiver->q_sending) 
        {
			p_from = receiver->q_sending;
			/* ready for p_from->hold_msg -> m */
			recv_ok = 1;

 			assert(receiver->p_status == 0);
			assert(receiver->p_hold_msg == 0);
			assert(receiver->p_want_recvfrom == NO_TASK);
			assert(receiver->p_want_sendto == NO_TASK);
			assert(receiver->q_sending != 0);
			assert(p_from->p_status == SENDING);
			assert(p_from->p_hold_msg != 0);
			assert(p_from->p_want_recvfrom == NO_TASK);
			assert(p_from->p_want_sendto == proc2pid(receiver));
		}
	}
	else if(src >= 0 && src < NR_TASKS+NR_PROCS)
    {
		/* receiver wants to receive a message from
		 * a certain proc: src.
		 */
		p_from = &proc_table[src];

 		if ((p_from->p_status & SENDING) &&
		    (p_from->p_want_sendto == proc2pid(receiver))) 
        {
			/* Perfect, src is sending a message to
			 * receiver.
			 */
			/* ready for p_from->hold_msg -> m */
			recv_ok = 1;

 			PROCESS* p = receiver->q_sending;
			assert(p); /* p_from must have been appended to the
				        * queue, so the queue must not be NULL
				        */
            
            /* 在receiver的q_sender队列中寻找指定的消息 */
			while (p)
            {
				assert(p_from->p_status & SENDING);
				if (proc2pid(p) == src)
                { /* if p is the one */
					p_from = p;
					break;
				}
				prev = p;
				p = p->next_sending;
			}

 			assert(receiver->p_status == 0);
			assert(receiver->p_hold_msg == 0);
			assert(receiver->p_want_recvfrom == NO_TASK);
			assert(receiver->p_want_sendto == NO_TASK);
			assert(receiver->q_sending != 0);
			assert(p_from->p_status == SENDING);
			assert(p_from->p_hold_msg != 0);
			assert(p_from->p_want_recvfrom == NO_TASK);
			assert(p_from->p_want_sendto == proc2pid(receiver));
		}
	}
	/**
	 * @Note:
	 * 这里我没有处理其他src,
	 * 例如当src = INTERRUPT时,
	 * copyok = 0,我直接让下面的代码顺便处理了, 这样做可能会有问题, 你可以此处补充些许代码手动处理下.
	 */

 	if (recv_ok) 
    {
		/* It's determined from which proc the message will
		 * be copied. Note that this proc must have been
		 * waiting for this moment in the queue, so we should
		 * remove it from the queue.
		 */
		if (p_from == receiver->q_sending) { /* the 1st one */
			assert(prev == 0);
			receiver->q_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}
		else {
			assert(prev);
			prev->next_sending = p_from->next_sending;
			p_from->next_sending = 0;
		}

 		assert(m);
		assert(p_from->p_hold_msg);
		/* copy the message */
		memcpy(va2la(proc2pid(receiver), m),
			  va2la(proc2pid(p_from), p_from->p_hold_msg),
			  sizeof(MESSAGE));

 		p_from->p_hold_msg = 0;
		p_from->p_want_sendto = NO_TASK;
		p_from->p_status &= ~SENDING;
		unblock(p_from);
	}
	else 
	{
		/**
		 * nobody's sending any msg.
		 * so, make this process in RECEIVING status 
		 * and lock it.
		 */
		receiver->p_status |= RECEIVING;

 		receiver->p_hold_msg = m;

		receiver->p_want_recvfrom = src;

 		block(receiver);

		assert(receiver->p_status == RECEIVING);
		assert(receiver->p_hold_msg != 0);
		assert(receiver->p_want_recvfrom != NO_TASK);
		assert(receiver->p_want_sendto == NO_TASK);
		//printf("receiver:%d, msg_from:%d\n",current-proc_table,src);
		/**@ Amazing!
		 * when cotrol flow run in pritf(), a HD interrupt happen
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
		//assert(receiver->has_int_msg == 0);
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
	PROCESS *p = proc_table + task_nr;

 	if ((p->p_status & RECEIVING) && /* dest is waiting for the msg */
	    ((p->p_want_recvfrom == INTERRUPT) || (p->p_want_recvfrom == ANY))) 
	{
		p->p_hold_msg->source 	= INTERRUPT;
		p->p_hold_msg->type 	= HARD_INT;
		/**Q: p_hold_msg为什么要清零?
 		 * A: 因为p_dest已经"收到了"想要的msg, 接下来unlock(p_dest),
		 * 	  p_dest就会正式接收处理这个msg.
		 * 	  所以, p_dest不在处于RECEIVING状态而保持(hold)msg了.
		 */
		p->p_hold_msg 			= 0;
		p->has_int_msg 			= 0;
		p->p_status 			&= ~RECEIVING; /* dest has received the msg */
		p->p_want_recvfrom 		= NO_TASK;

 		assert(p->p_status == 0);

 		unblock(p);

 		assert(p->p_status == 0);
		assert(p->p_hold_msg == 0);
		assert(p->p_want_recvfrom == NO_TASK);
		assert(p->p_want_sendto == NO_TASK);
	}
	else
	{
		p->has_int_msg = 1;
		/* 当 p 调用msg_receive()时,会优先检查自己是否有硬件消息(has_int_msg) */
	}
}



/** =========================== block ============================
 * <Ring 0> This routine is called after `p_status' has been set (!= 0), it
 * calls `schedule()' to choose another proc as the `proc_ready'.
 * When Ring adjusts from 0 -> 1-3, the restart() will switch eip to "ready process". 
 *
 * @attention This routine does not change `p_status'. Make sure the `p_status'
 * of the proc to be blocked has been set properly.
 * 
 * @param p The proc to be blocked.
 */
PRIVATE void block(PROCESS* p)
{
	assert(p->p_status);
	schedule();
}

/** ======================== unblock ========================
 * <Ring 0> This is a dummy routine. It does nothing actually. When it is
 * called, the `p_status' should have been cleared (== 0).
 * 
 * @param p The unblocked proc.
 */
PRIVATE void unblock(PROCESS* p)
{
    assert(p->p_status == 0);
}

/** =========================== deadlock ==============================
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
 */
PRIVATE int deadlock(int src, int dest)
{
	PROCESS* p = proc_table + dest;
	while (1) 
    {
		if (p->p_status & SENDING) 
        {
			if (p->p_want_sendto == src) 
            {
				/* print the chain */
				p = proc_table + dest;
				printf("=_=%s", p->name);
				do {
					assert(p->p_hold_msg);
					p = proc_table + p->p_want_sendto;
					printf("->%s", p->name);
				} while (p != proc_table + src);
				printf("=_=");

 				return 1;
			}
			p = proc_table + p->p_want_sendto;
		}
		else 
        {
            /* 如果目的process不处于发送状态, 也就不可能形成消息死锁 */
			break;
		}
	}
	return 0;
}




/** ============= dump_proc ===============
 * 显示进程的信息
 */
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

 	/* printf("^^"); */

 	disp_color_str("\n\n", text_color);
	sprintf(info, "ANY: 0x%x.\n", ANY); disp_color_str(info, text_color);
	sprintf(info, "NO_TASK: 0x%x.\n", NO_TASK); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);

 	sprintf(info, "ldt_sel: 0x%x.  ", p->ldt_sel); disp_color_str(info, text_color);
	sprintf(info, "ticks: 0x%x.  ", p->ticks); disp_color_str(info, text_color);
	sprintf(info, "priority: 0x%x.  ", p->priority); disp_color_str(info, text_color);
	sprintf(info, "pid: 0x%x.  ", p->pid); disp_color_str(info, text_color);
	sprintf(info, "name: %s.  ", p->name); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);
	sprintf(info, "p_status: 0x%x.  ", p->p_status); disp_color_str(info, text_color);
	sprintf(info, "p_want_recvfrom: 0x%x.  ", p->p_want_recvfrom); disp_color_str(info, text_color);
	sprintf(info, "p_want_sendto: 0x%x.  ", p->p_want_sendto); disp_color_str(info, text_color);
	sprintf(info, "nr_tty: 0x%x.  ", p->nr_tty); disp_color_str(info, text_color);
	disp_color_str("\n", text_color);
	sprintf(info, "has_int_msg: 0x%x.  ", p->has_int_msg); disp_color_str(info, text_color);
}



/** =================== dump_msg ===================
 * 显示msg的内容
 */
PUBLIC void dump_msg(const char * title, MESSAGE* m)
{
	int packed = 0;
	printf("{%s}<0x%x>{%ssrc:%s(%d),%stype:%d,%s(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x)%s}%s",  //, (0x%x, 0x%x, 0x%x)}",
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
