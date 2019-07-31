



#include "type.h"
#include "const.h"
#include "struct_hd.h"
#include "struct_proc.h"
#include "const_interrupt.h"
#include "func_proto.h"
#include "struct_fs.h"



PRIVATE void	init_hd();
PRIVATE void	hd_open(int device);
PRIVATE void	hd_close(int device);
PRIVATE void	hd_rdwt(MESSAGE * p);
PRIVATE void	hd_ioctl(MESSAGE * p);
PRIVATE void    hd_cmd_out(struct hd_cmd* cmd);
PRIVATE void	get_part_table(int drive, int sect_nr, struct part_ent * entry);
PRIVATE void	partition(int device, int style);
PRIVATE void	print_hdinfo(struct hd_info * hdi);
PRIVATE int		waitfor(int mask, int val, int timeout);
PRIVATE void	interrupt_wait();
PRIVATE	void	hd_identify(int drive);
PRIVATE void	print_identify_info(u16* hdinfo);

#define	DRV_OF_DEV(dev) (dev <= MAX_PRIM ? \
                 dev / NR_PRIM_PER_HD_DRIVE : \
			 (dev - MINOR_hd1a) / NR_LG_PARTS_PER_HD_DRIVE)


 /* 内部要用的的暂时数据 */
PRIVATE	u8				hd_status;

/* 读硬盘数据时,用来存储从硬盘读出的数据 */
PRIVATE	u8				hdbuf[SECTOR_SIZE * 2];

/* hd_info结构中含有硬盘中主分区和扩展分区的起始扇区和扇区数量 */
PRIVATE	struct hd_info	hd_info[1];


/*****************************************************************************
 *                                hd_handler
 *****************************************************************************/
/**
 * <Ring 0> Interrupt handler.
 * 
 * @param irq  IRQ nr of the disk interrupt.
 *****************************************************************************/
PUBLIC void hd_handler(int irq)
{
	/*
	 * Interrupts are cleared when the host
	 *   - reads the Status Register,
	 *   - issues a reset, or
	 *   - writes to the Command Register.
	 */
		//printf("hd_handler() wait for REG_STATUS\n");
	hd_status = in_byte(REG_STATUS);
		//printf("hd_handler() come\n");
	inform_int(TASK_HD);

 		//printf("hd_handler() over\n");
}


 /*****************************************************************************
 *                                task_hd
 *****************************************************************************/
/**
 * Main loop of HD driver.
 * 
 *****************************************************************************/
PUBLIC void task_hd()
{
	MESSAGE msg;

 	init_hd();	/* 启动硬盘中断 */

 	while (1)
	{
		send_recv(RECEIVE, ANY, &msg);

 		int src = msg.source;

 		switch (msg.type)
		{
		case DEV_OPEN:
			hd_open(msg.DEVICE);
			break;

        case DEV_CLOSE:
			hd_close(msg.DEVICE);
			break;

 		case DEV_READ:
		case DEV_WRITE:
			hd_rdwt(&msg);
			break;

 		case DEV_IOCTL:
			hd_ioctl(&msg);
			break;

 		default:
			dump_msg("HD driver::unknown msg", &msg);
			spin("FS::main_loop (invalid msg.type)");
			break;
		}

 		send_recv(SEND, src, &msg);
	}
}



 /*****************************************************************************
 *                                init_hd
 *****************************************************************************/
/**
 * <Ring 1> Check hard drive, set IRQ handler, enable IRQ and initialize data
 *          structures.
 *****************************************************************************/
PRIVATE void init_hd()
{
	/* Get the number of drives from the BIOS data area */
	u8 * pNrDrives = (u8*)(0x475);
	printf("NrDrives:%d.\n", *pNrDrives);
	assert(*pNrDrives);

 	put_irq_handler(AT_WINI_IRQ, hd_handler);
	enable_irq(CASCADE_IRQ);
	enable_irq(AT_WINI_IRQ);

    /* 将所有硬盘的信息清零 */
	for(int i = 0; i < (sizeof(hd_info) / sizeof(hd_info[0])); i++)
	{
		memset(&hd_info[i], 0, sizeof(hd_info[0]));
	}
	hd_info[0].open_cnt = 0;
}





 /*****************************************************************************
 *                                hd_open
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_OPEN message. It identify the drive
 * of the given device and read the partition table of the drive if it
 * has not been read.
 * 
 * @param device The device to be opened.
 *****************************************************************************/
PRIVATE void hd_open(int device)
{
	int drive = DRV_OF_DEV(device);
	assert(drive == 0);	/* only one drive */

 	hd_identify(drive);

 	if (hd_info[drive].open_cnt++ == 0)
	{
		//printf("start to call partition()\n");
		partition(drive * (NR_PARTS_PER_HD_HRIVE + 1), P_PRIMARY);
		//printf("start to invoke print_hdinfo()\n");
		print_hdinfo(&hd_info[drive]);
	}
}


/*****************************************************************************
 *                                hd_close
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_CLOSE message. 
 * 
 * @param device The device to be opened.
 *****************************************************************************/
PRIVATE void hd_close(int device)
{
	int drive = DRV_OF_DEV(device);
	assert(drive == 0);	/* only one drive */

 	hd_info[drive].open_cnt--;
}



 /*****************************************************************************
 *                                hd_rdwt
 *****************************************************************************/
/**
 * <Ring 1> This routine handles DEV_READ and DEV_WRITE message.
 * 
 * @param p_msg Message ptr.
 *****************************************************************************/
PRIVATE void hd_rdwt(MESSAGE * p_msg)
{
	int drive = DRV_OF_DEV(p_msg->DEVICE);

 	u64 pos = p_msg->POSITION;
	assert((pos >> SECTOR_SIZE_SHIFT) < (1 << 31));

 	/**
	 * We only allow to R/W from a SECTOR boundary:
	 */
	assert((pos & 0x1FF) == 0);

 	u32 sect_nr = (u32)(pos >> SECTOR_SIZE_SHIFT); /* pos / SECTOR_SIZE */
	int logidx = (p_msg->DEVICE - MINOR_hd1a) % NR_LG_PARTS_PER_HD_DRIVE;
	sect_nr += p_msg->DEVICE < MAX_PRIM ?
		hd_info[drive].primary[p_msg->DEVICE].base :
		hd_info[drive].logical[logidx].base;

 	struct hd_cmd cmd;
	cmd.features	= 0;
	cmd.count	= (p_msg->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
	cmd.lba_low	= sect_nr & 0xFF;
	cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xF);
	cmd.command	= (p_msg->type == DEV_READ) ? ATA_READ : ATA_WRITE;
	hd_cmd_out(&cmd);

 	int bytes_left = p_msg->CNT;
	void * la = (void*)va2la(p_msg->PROC_NR, p_msg->BUF);

 	while (bytes_left) {
		int bytes = min(SECTOR_SIZE, bytes_left);
		if (p_msg->type == DEV_READ) {
			interrupt_wait();
			port_read(REG_DATA, hdbuf, SECTOR_SIZE);
			memcpy(la, (void*)va2la(TASK_HD, hdbuf), bytes);
		}
		else {
			if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT))
				panic("hd writing error.");

 			port_write(REG_DATA, la, bytes);
			interrupt_wait();
		}
		bytes_left -= SECTOR_SIZE;
		la += SECTOR_SIZE;
	}
}															      



 /*****************************************************************************
 *                                hd_ioctl
 *****************************************************************************/
/**
 * <Ring 1> This routine handles the DEV_IOCTL message.
 * 
 * @param p_msg  Ptr to the MESSAGE.
 *****************************************************************************/
PRIVATE void hd_ioctl(MESSAGE * p_msg)
{
	int device = p_msg->DEVICE;
	int drive = DRV_OF_DEV(device);

 	struct hd_info * hdi = &hd_info[drive];

 	if (p_msg->REQUEST == DIOCTL_GET_GEO) {
		void * dst = va2la(p_msg->PROC_NR, p_msg->BUF);
		void * src = va2la(TASK_HD,
				   device < MAX_PRIM ?
				   &hdi->primary[device] :
				   &hdi->logical[(device - MINOR_hd1a) %
						NR_LG_PARTS_PER_HD_DRIVE]);

 		memcpy(dst, src, sizeof(struct part_info));
	}
	else {
		assert(0);
	}
}


 /*****************************************************************************
 *                                get_part_table
 *****************************************************************************/
/**
 * <Ring 1> Get a partition table of a drive.
 * 
 * @param drive   Drive nr (0 for the 1st disk, 1 for the 2nd, ...)n
 * @param sect_nr The sector at which the partition table is located.
 * @param entry   Ptr to part_ent struct.
 *****************************************************************************/
PRIVATE void get_part_table(int drive, int sect_nr, struct part_ent * entry)
{
	struct hd_cmd cmd;
	cmd.features	= 0;
	cmd.count	= 1;
	cmd.lba_low	= sect_nr & 0xFF;
	cmd.lba_mid	= (sect_nr >>  8) & 0xFF;
	cmd.lba_high	= (sect_nr >> 16) & 0xFF;
	cmd.device	= MAKE_DEVICE_REG(1, /* LBA mode*/
					  drive,
					  (sect_nr >> 24) & 0xF);
	cmd.command	= ATA_READ;
	hd_cmd_out(&cmd);
		//printf("get_part_table()-1 : call intertupt_wait()\n");
		//milli_delay(10); 
		/**
		 * @Phenomenon:
		 * Here is a very strange bug...
		 * If you invoke milidelay(),
		 * the IPC moudle -> msg_receive() : assert(p_who_wanna_recv->has_int_msg == 0)
		 * will cause fault...
		 * 
		 * @Explanation:
		 * When you call mili_dalay(),after msg_send(task_hd,task_sys,msg)
		 * exec-flow comes to msg_receive(task_hd,task_sys,msg) to get result of get_ticks()
		 * and in msg_receive(...) the hard disk interrupt and set 
		 */
	interrupt_wait();

 		//printf("get_part_table()-2 : get msg and call port_read()\n");
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);
		//printf("get_part_table()-3 : after call port_read()\n");
	memcpy(entry,
	       hdbuf + PARTITION_TABLE_OFFSET,
	       sizeof(struct part_ent) * NR_PARTS_PER_HD_HRIVE);
}

 /*****************************************************************************
 *                                partition
 *****************************************************************************/
/**
 * <Ring 1> This routine is called when a device is opened. It reads the
 * partition table(s) and fills the hd_info struct.
 * 
 * @param device Device nr.
 * @param style  P_PRIMARY or P_EXTENDED.
 *****************************************************************************/
PRIVATE void partition(int device, int style)
{
	int drive = DRV_OF_DEV(device);
	struct hd_info * hdi = &hd_info[drive];

 	/* 硬盘分区表 */
	struct part_ent part_tbl[NR_LG_PARTS_PER_HD_DRIVE];

 	if (style == P_PRIMARY)
	{
			//printf("master partition() invoke get_part_table()\n");
		get_part_table(drive, drive, part_tbl);
			//printf("master partition() over get_part_table()\n");

 		/* 主分区个数 */
		int nr_prim_parts = 0;
		for (int i = 0; i < NR_PARTS_PER_HD_HRIVE; i++) 
		{ 	/* 0~3 */
			if (part_tbl[i].sys_id == NO_PART) 
				continue;

 			nr_prim_parts++;
			int dev_nr = i + 1;		  /* 1~4, 因为hd_info[0]表示的是整块硬盘,不是分区 */
			hdi->primary[dev_nr].base = part_tbl[i].start_sect;
			hdi->primary[dev_nr].size = part_tbl[i].nr_sects;

 			if (part_tbl[i].sys_id == EXT_PART) /* extended */
			{
					//printf("partition() handle P_EXTENDED\n");
				partition(device + dev_nr, P_EXTENDED);
			}
		}

 		/* 主分区个数至少为1 */
		assert(nr_prim_parts != 0);

 			//printf("partition() master P over.\n");
	}
	else if (style == P_EXTENDED) 
	{
		int j = device % NR_PRIM_PER_HD_DRIVE; /* 1~4 */
		int ext_start_sect = hdi->primary[j].base;
		int s = ext_start_sect;
		int nr_1st_sub = (j - 1) * NR_LG_PARTS_PER_EX_PART; /* 0/16/32/48 */

 		for (int i = 0; i < NR_LG_PARTS_PER_EX_PART; i++)
		{
			int dev_nr = nr_1st_sub + i;/* 0~15/16~31/32~47/48~63 */
				//printf("extended partition() invoke get_part_table()\n");
			get_part_table(drive, s, part_tbl);

 			hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
			hdi->logical[dev_nr].size = part_tbl[0].nr_sects;

 			s = ext_start_sect + part_tbl[1].start_sect;

 			/* no more logical partitions
			   in this extended partition */
			if (part_tbl[1].sys_id == NO_PART)
			{
				//printf("partition() extended P over.\n");
				break;
			}
		}
	}
	else 
	{
		assert(0);
	}
}

 /*****************************************************************************
 *                                print_hdinfo
 *****************************************************************************/
/**
 * <Ring 1> Print disk info.
 * 
 * @param hdi  Ptr to struct hd_info.
 *****************************************************************************/
PRIVATE void print_hdinfo(struct hd_info * hdi)
{
	int i;
	for (i = 0; i < NR_PARTS_PER_HD_HRIVE + 1; i++)
	{
		printf("%sPART_%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		       i == 0 ? " " : "     ",
		       i,
		       hdi->primary[i].base,
		       hdi->primary[i].base,
		       hdi->primary[i].size,
		       hdi->primary[i].size);
	}
	for (i = 0; i < NR_LG_PARTS_PER_HD_DRIVE; i++)
	{
		if (hdi->logical[i].size == 0)
			continue;
		printf("         "
		       "%d: base %d(0x%x), size %d(0x%x) (in sector)\n",
		       i,
		       hdi->logical[i].base,
		       hdi->logical[i].base,
		       hdi->logical[i].size,
		       hdi->logical[i].size);
	}
}


 /*****************************************************************************
 *                                hd_identify
 *****************************************************************************/
/**
 * <Ring 1> Get the disk information.
 * 
 * @param drive  Drive Nr.
 *****************************************************************************/
PRIVATE void hd_identify(int drive)
{
	struct hd_cmd cmd;
	cmd.device  = MAKE_DEVICE_REG(0, drive, 0);
	cmd.command = ATA_IDENTIFY;
	hd_cmd_out(&cmd);
	interrupt_wait();
	port_read(REG_DATA, hdbuf, SECTOR_SIZE);

 	print_identify_info((u16*)hdbuf);

    /* hd_info[0]代表整块硬盘 */
	u16* hdinfo = (u16*)hdbuf;
	hd_info[drive].primary[0].base = 0;
	/* Total Nr of User Addressable Sectors */
	hd_info[drive].primary[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
}

 /*****************************************************************************
 *                            print_identify_info
 *****************************************************************************/
/**
 * <Ring 1> Print the hdinfo retrieved via ATA_IDENTIFY command.
 * 
 * @param hdinfo  The buffer read from the disk i/o port.
 *****************************************************************************/
PRIVATE void print_identify_info(u16* hdinfo)
{
	int i, k;
	char s[64];

 	struct iden_info_ascii 
	{
		int idx;
		int len;
		char * desc;
	} iinfo[] = {
		{10, 20, "HD SN"}, 	/* Serial number in ASCII */
		{27, 40, "HD Model"}/* Model number in ASCII */ 
	};

 	for (k = 0; k < sizeof(iinfo)/sizeof(iinfo[0]); k++) 
	{
		char * p = (char*)&hdinfo[iinfo[k].idx];
		for (i = 0; i < iinfo[k].len/2; i++)
		{
			s[i*2+1] = *p++;
			s[i*2] = *p++;
		}
		s[i*2] = 0;
		printf("%s: %s\n", iinfo[k].desc, s);
	}

 	int capabilities = hdinfo[49];
	printf("LBA supported: %s\n",
	       (capabilities & 0x0200) ? "Yes" : "No");

 	int cmd_set_supported = hdinfo[83];
	printf("LBA48 supported: %s\n",
	       (cmd_set_supported & 0x0400) ? "Yes" : "No");

 	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	printf("HD size: %dMB\n", sectors * 512 / 1000000);
}

 /*****************************************************************************
 *                                hd_cmd_out
 *****************************************************************************/
/**
 * <Ring 1> Output a command to HD controller.
 * 
 * @param cmd  The command struct ptr.
 *****************************************************************************/
PRIVATE void hd_cmd_out(struct hd_cmd* cmd)
{
	/**
	 * For all commands, the host must first check if BSY=1,
	 * and should proceed no further unless and until BSY=0
	 */
	if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT))
		panic("hd error.");

 	/* Activate the Interrupt Enable (nIEN) bit */
	out_byte(REG_DEV_CTRL, 0);
	/* Load required parameters in the Command Block Registers */
	out_byte(REG_FEATURES, cmd->features);
	out_byte(REG_NSECTOR,  cmd->count);
	out_byte(REG_LBA_LOW,  cmd->lba_low);
	out_byte(REG_LBA_MID,  cmd->lba_mid);
	out_byte(REG_LBA_HIGH, cmd->lba_high);
	out_byte(REG_DEVICE,   cmd->device);
	/* Write the command code to the Command Register */
	out_byte(REG_CMD,     cmd->command);
}

 /*****************************************************************************
 *                                interrupt_wait
 *****************************************************************************/
/**
 * <Ring 1> Wait until a disk interrupt occurs.
 * 
 *****************************************************************************/
PRIVATE void interrupt_wait()
{
	MESSAGE msg;
	send_recv(RECEIVE, INTERRUPT, &msg);
}

 /*****************************************************************************
 *                                waitfor
 *****************************************************************************/
/**
 * <Ring 1> Wait for a certain status.
 * 
 * @param mask    Status mask.
 * @param val     Required status.
 * @param timeout Timeout in milliseconds.
 * 
 * @return One if sucess, zero if timeout.
 *****************************************************************************/
PRIVATE int waitfor(int mask, int val, int timeout)
{
	int t = get_ticks();

 	while(((get_ticks() - t) * 1000 / HZ) < timeout)
	{
		if ((in_byte(REG_STATUS) & mask) == val)
		{
			return 1;
		}
	}

 	return 0;
}

