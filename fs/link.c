


#include "proto.h"
#include "string.h"
#include "type.h"
#include "stdio.h"
#include "global.h"
#include "const.h"


/*****************************************************************************
 *                                do_unlink
 *****************************************************************************/
/**
 * Remove a file.
 *
 * @note We clear the i-node in inode_array[] although it is not really needed.
 *       We don't clear the data bytes so the file is recoverable.
 * 
 * @return On success, zero is returned.  On error, -1 is returned.
 *****************************************************************************/
PUBLIC int do_unlink()
{
	char pathname[MAX_PATH];

	/* get parameters from the message */
	int name_len 	= fs_msg.NAME_LEN;	/* length of filename */
	int src 		= fs_msg.source;	/* caller proc nr. */
	assert(name_len < MAX_PATH);

	memcpy((void*)va2la(TASK_FS, pathname),
		  	(void*)va2la(src, fs_msg.PATHNAME),
		  	name_len);
	pathname[name_len] = 0;

	if (strcmp(pathname , "/") == 0) 
	{
		printl("FS:do_unlink():: cannot unlink the root\n");
		return -1;
	}

	// 获取pathname('/XXX')对应的iNode的索引
	int inode_nr = search_file(pathname);
	if (inode_nr == INVALID_INODE)
	{	/* file not found */
		printl("FS::do_unlink():: search_file() returns "
			"invalid inode: %s\n", pathname);
		return -1;
	}

	char filename[MAX_PATH];
	// 获取pathname('/XXX')对应的目录('/')的iNode的指针
	struct inode * dir_inode;
	if (strip_path(filename, pathname, &dir_inode) != 0)
		return -1;

	// 获取pathname('/xxx')对应的iNode的指针
	struct inode * pin = get_inode(dir_inode->i_dev, inode_nr);

	if (pin->i_mode != I_REGULAR) 
	{ /* can only remove regular files */
		printl("cannot remove file %s, because "
		       "it is not a regular file.\n",
		       pathname);
		return -1;
	}

	if (pin->i_cnt > 1) 
	{	/* the file was opened by another process */
		printl("cannot remove file %s, because pin->i_cnt is %d.\n",
		       pathname, pin->i_cnt);
		return -1;
	}

	struct super_block * sb = get_super_block(pin->i_dev);

	/*************************/
	/* free the bit in i-map */
	/*************************/
	int byte_idx 	= inode_nr / 8;
	int bit_idx		= inode_nr % 8;
	assert(byte_idx < SECTOR_SIZE);	/* we have only one i-map sector */
	
	RD_SECT(pin->i_dev, 2); /* read sector 2 (skip bootsect and superblk): */

	assert(fsbuf[byte_idx] & (1 << bit_idx));

	fsbuf[byte_idx] &= ~(1 << bit_idx); //清0
	WR_SECT(pin->i_dev, 2);

	/**************************/
	/* free the bits in s-map */
	/**************************/
	bit_idx  		= pin->i_start_sect - sb->n_1st_sect + 1; //iNode在smap中的bit位位置(bit位) //@注意: smap和imap的第一位都是保留位
	byte_idx 		= bit_idx / 8;							  //iNode在smap中bit位所在位置(字节)

	int bits_left 	= pin->i_nr_sects; 						  //iNode对应文件占据的扇区数目(需要释放的smap中的b总it位数)
	int byte_cnt 	= (bits_left - (8 - (bit_idx % 8))) / 8;  //iNode对应文件占据的扇区数目(字节数,不包括第一个字节和最后一个字节) //@注意: 如果最后一个字节是0xff,则包括在内

	/* current smap sector nr. */
	int current_smap_sector = 2  /* 2: bootsect + superblk */
							+ sb->nr_imap_sects + byte_idx / SECTOR_SIZE;

	// 读取smap的第一个扇区
	RD_SECT(pin->i_dev, current_smap_sector);

	/* clear the first byte */
	for (int i = bit_idx % 8; (i < 8) && bits_left;	i++, bits_left--)
	{
		//i: 第一个字节中占据的bit位数
		// 7... i...0
		// [?..?10..0]
		assert((fsbuf[byte_idx % SECTOR_SIZE] >> i) & 1);
		fsbuf[byte_idx % SECTOR_SIZE] &= ~(1 << i);
	}

	/* clear bytes from the second byte to the second to last */
	int byte_idx_in_sector = (byte_idx % SECTOR_SIZE) + 1;	/* the second byte in a sector*/
	for (int k = 0; k < byte_cnt; k++, byte_idx_in_sector++, bits_left-=8) 
	{
		if (byte_idx_in_sector == SECTOR_SIZE) //当 byte_idx_in_sector 越过扇区边缘,跨入下一个扇区时(idx: 0~511)
		{
			byte_idx_in_sector = 0; //idx = 下一个扇区的第一个字节(0)
			// 上一个扇区smap写入硬盘
			WR_SECT(pin->i_dev, current_smap_sector);
			// 读取smap的下一个扇区
			RD_SECT(pin->i_dev, ++current_smap_sector);
		}
		
		assert(fsbuf[byte_idx_in_sector] == 0xFF);
		fsbuf[byte_idx_in_sector] = 0;
	}
	//当 byte_idx_in_sector 没有越过扇区边缘,只是写入扇区最后一个字节,而不写入下一个扇区时,上一个for-loop会由于byte_cnt限制而跳出循环来到这里
	if (byte_idx_in_sector == SECTOR_SIZE)
	{
		byte_idx_in_sector = 0;
		WR_SECT(pin->i_dev, current_smap_sector);
		RD_SECT(pin->i_dev, ++current_smap_sector);
	}

	// clear the last byte(!= 0xff)
	unsigned char mask = ~((unsigned char)(~0) << bits_left);
	assert((fsbuf[byte_idx_in_sector] & mask) == mask);
	fsbuf[byte_idx_in_sector] &= (~0) << bits_left;
	WR_SECT(pin->i_dev, current_smap_sector);

	/***************************/
	/* clear the i-node itself */
	/***************************/
	pin->i_mode = 0;
	pin->i_size = 0;
	pin->i_start_sect = 0;
	pin->i_nr_sects = 0;
	sync_inode(pin);
	/* release slot in inode_table[] */
	put_inode(pin);

	/************************************************/
	/* set the inode-nr to 0(means invalid inode) in the directory entry */
	/************************************************/
	int dir_blk0_nr 	= dir_inode->i_start_sect;
	int nr_dir_blks 	= (dir_inode->i_size + SECTOR_SIZE) / SECTOR_SIZE;
	int nr_dir_entries 	= dir_inode->i_size / DIR_ENTRY_SIZE; /* including unused slots
						    				 			       * (the file has been
						     								   * deleted but the slot
						    								   * is still there)
						     								   */
	int nr_searched_dir_entry= 0; //已经搜索过的目录项数目
	struct dir_entry * pde 	= 0;
	int flg 				= 0; //1: file's iNode is found, 0: file's iNode isn't found.
	int new_dir_size 		= 0; //在删除文件的目录项后,目录的大小

	for (int i = 0; i < nr_dir_blks; i++) //遍历含有目录项的所有扇区
	{
		// 读取目录项所在扇区
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);

		pde = (struct dir_entry *)fsbuf;
		for (int j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++) //遍历扇区中的所有目录项
		{
			if (++nr_searched_dir_entry > nr_dir_entries)
				break;

			if (pde->inode_nr == inode_nr) //found!
			{
				/* pde->inode_nr = 0; */
				memset(pde, 0, DIR_ENTRY_SIZE);
				WR_SECT(dir_inode->i_dev, dir_blk0_nr + i);
				flg = 1;
				break;
			}

			if (pde->inode_nr != INVALID_INODE)
				new_dir_size += DIR_ENTRY_SIZE;
		}

		if (nr_searched_dir_entry > nr_dir_entries || /* all entries have been iterated OR */
			flg)   									  /* file is found */
			break;
	}

	assert(flg);

	if (nr_searched_dir_entry == nr_dir_entries) /* the file is the last one in the dir */
	{
		dir_inode->i_size = new_dir_size;
		sync_inode(dir_inode);
		/**
		 * @note
		 * 通常情况下(dir_node指向的由dir_entry构成的文件,在删除一个目录项后会形成空洞),我们不更新dir_node的大小
		 * 这意味着i_size != 实际目录项集合的大小
		 */
	}

	return 0;
}
