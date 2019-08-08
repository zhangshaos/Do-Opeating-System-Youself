



#include "proto.h"
#include "string.h"
#include "type.h"
#include "global.h"
#include "const.h"
#include "stdio.h"



/*****************************************************************************
 *                                do_rdwt
 *****************************************************************************/
/**
 * Read/Write file and return byte count read/written.
 *
 * Sector map is not needed to update, since the sectors for the file have been
 * allocated and the bits are set when the file was created.
 * 
 * @return How many bytes have been read/written.
 *****************************************************************************/
PUBLIC int do_rdwt()
{
	int fd 		= fs_msg.FD;	/* < file descriptor. */
	void * buf 	= fs_msg.BUF;	/* < r/w buffer */
	int len 	= fs_msg.CNT;	/* < r/w bytes */
	int src	 	= fs_msg.source;/* caller proc nr. */

	assert((pcaller->filp[fd] >= &f_desc_table[0]) &&
	       (pcaller->filp[fd] < &f_desc_table[NR_FILE_DESC]));

	if (!(pcaller->filp[fd]->fd_mode & O_RDWR))
		return 0;

	int pos = pcaller->filp[fd]->fd_pos;

	struct inode * pin = pcaller->filp[fd]->fd_inode;

	assert(pin >= &inode_table[0] && pin < &inode_table[NR_INODE]);

	int imode = pin->i_mode & I_TYPE_MASK;

// 如果inode是字符设备
	if (imode == I_CHAR_SPECIAL) 
	{
		int t = fs_msg.type == READ ? DEV_READ : DEV_WRITE;
		fs_msg.type 	= t;

		int dev = pin->i_start_sect;
		assert(MAJOR(dev) == 4);

		fs_msg.DEVICE	= MINOR(dev);
		fs_msg.BUF		= buf;
		fs_msg.CNT		= len;
		fs_msg.PROC_NR	= src;
		assert(dd_map[MAJOR(dev)].driver_nr != INVALID_DRIVER);
		send_recv(BOTH, dd_map[MAJOR(dev)].driver_nr, &fs_msg);
		assert(fs_msg.CNT == len);

		return fs_msg.CNT;
	}
	else 
	{
		// iNode是一般文件
		assert(pin->i_mode == I_REGULAR || pin->i_mode == I_DIRECTORY);
		assert((fs_msg.type == READ) || (fs_msg.type == WRITE));

		int pos_end;
		if (fs_msg.type == READ)
			pos_end = min(pos + len, pin->i_size);	//pin->size:文件的真实大小
		else		/* WRITE */
			pos_end = min(pos + len, pin->i_nr_sects * SECTOR_SIZE);	//pin->i_nr_sects * SECTOR*SECTOR_SIZE:包括文件预留扇区的文件大小

		int off 		= pos % SECTOR_SIZE;	//读写文件时,读写指针pos距离上一个扇区(512字节)的偏移字节
		int rw_sect_min	= pin->i_start_sect+(pos>>SECTOR_SIZE_SHIFT);	//读写文件pos位置时时,文件的起始扇区
		int rw_sect_max	= pin->i_start_sect+(pos_end>>SECTOR_SIZE_SHIFT);	//读写文件pos位置时,文件的最后一个扇区

// 读写文件时,按照chunk块扇区大小读写(但是一次读写不可以超过fsbuf缓冲的大小)
		int chunk = min(rw_sect_max - rw_sect_min + 1,
						FSBUF_SIZE >> SECTOR_SIZE_SHIFT);

		int bytes_rw = 0;	//已经读写的字节数
		int bytes_left = len;	//还未读写的字节数
		for (int i = rw_sect_min; i <= rw_sect_max; i += chunk) 
		{
			// bytes:此次读写文件的字节数
			int bytes = min(bytes_left, chunk * SECTOR_SIZE - off);
			// 读取chunk块扇区先()
			rw_sector(DEV_READ,
				  	pin->i_dev,
				  	i * SECTOR_SIZE,
				  	chunk * SECTOR_SIZE,
				  	TASK_FS,
				  	fsbuf);

			if (fs_msg.type == READ) 
			{
				// 将读取的文件内容复制到caller的缓冲区
				memcpy((void*)va2la(src, buf + bytes_rw),
					  (void*)va2la(TASK_FS, fsbuf + off),
					  bytes);
			}
			else 
			{	/* WRITE */
				// 将caller缓冲区文件内容复制到fsbuf+off处
				// 从fsbuf->fsbuf+off这段fs缓冲区由于预先读取,而填充了正确的文件内容
				memcpy((void*)va2la(TASK_FS, fsbuf + off),
					  (void*)va2la(src, buf + bytes_rw),
					  bytes);
				// 将fsbuf内容写入硬盘
				rw_sector(DEV_WRITE,
					  	pin->i_dev,
					  	i * SECTOR_SIZE,
					  	chunk * SECTOR_SIZE,
					  	TASK_FS,
					  	fsbuf);
			}

			off = 0;
			bytes_rw += bytes;
			pcaller->filp[fd]->fd_pos += bytes;
			bytes_left -= bytes;
		}

		// 当文件的读写指针超过文件的大小时,修改iNode的大小
		// (文件预留了一部分扇区,所以直接这么做没问题)
		if (pcaller->filp[fd]->fd_pos > pin->i_size) 
		{
			/* update inode::size */
			pin->i_size = pcaller->filp[fd]->fd_pos;
			/* write the updated i-node back to disk */
			sync_inode(pin);
		}

		return bytes_rw;
	}
}
