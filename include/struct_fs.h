

#ifndef	_ORANGES_FS_H_
#define	_ORANGES_FS_H_

#include "type.h"



#define	MAGIC_V1	0x111

#define	INVALID_INODE		0
#define	ROOT_INODE			1

#define ORANGES_PART	0x99	/* Orange'S partition */
#define NO_PART		    0x00	/* unused entry */
#define EXT_PART	    0x05	/* extended partition */

#define	NR_FILES	    64
#define	NR_FILE_DESC	64	/* FIXME */
#define	NR_INODE	    64	/* FIXME */
#define	NR_SUPER_BLOCK	8


 /* INODE::i_mode (octal, lower 12 bits reserved) */
#define I_TYPE_MASK     0170000
#define I_REGULAR       0100000
#define I_BLOCK_SPECIAL 0060000
#define I_DIRECTORY     0040000
#define I_CHAR_SPECIAL  0020000
#define I_NAMED_PIPE	0010000

 #define	is_special(m)	((((m) & I_TYPE_MASK) == I_BLOCK_SPECIAL) ||	\
			 (((m) & I_TYPE_MASK) == I_CHAR_SPECIAL))

 #define	NR_DEFAULT_FILE_SECTS	2048 /* 2048 * 512 = 1MB */









struct super_block
{
	u32	magic;		        /* magic number */
	u32	nr_inodes;	        /* total number of i-node */
	u32	nr_sects;	        /* total sectors of this hard disk */
	u32	nr_imap_sects;	    /* sectors occupyed by inode-map */
	u32	nr_smap_sects;	    /* sectors occupyed by sector-map */
	u32	n_1st_sect;	        /* the first data sector you can write */

 	u32	nr_inode_sects;     /* sectors occupyed by inode */
	u32	root_inode;         /* inode's index of root directory */
	u32	inode_size;       
	u32	inode_isize_off;    /* Offset of `struct inode::i_size' */
	u32	inode_start_off;    /* Offset of `struct inode::i_start_sect' */

	u32	dir_ent_size;       /* 目录区大小 */
	u32	dir_ent_inode_off;  /* Offset of `struct dir_entry::inode_nr' */
	u32	dir_ent_fname_off;  /* Offset of `struct dir_entry::name' */

 	/*
	 * the following item(s) are only present in memory
	 */
	int	sb_dev;     /*(该成员只出现在内存中) 记录此超级快对应哪快硬盘 */
};

 /**
 * @def   SUPER_BLK_MAGIC_V1
 * @brief Magic number of super block, version 1.
 * @attention It must correspond with boot/include/load.h::SB_MAGIC_V1
 */
#define	SUPER_BLK_MAGIC_V1		0x111

 /**
 * @def   SUPER_BLOCK_SIZE
 * @brief The size of super block \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define	SUPER_BLOCK_SIZE	56

 



struct inode 
{
	u32	i_mode;
	u32	i_size;		    /* file size */
	u32	i_start_sect;	/* first sector of the file */
	u32	i_nr_sects;     /* the sectors occupyed by the file */
	u8	__unused[16];	/* Stuff for alignment */

 	/* the following items are only present in memory */
	int	i_dev;      /* the device(hd) of this inode */
	int	i_cnt;		/* sharing thie inode's process number */
	int	i_num;      /* this inode's index */
};

 /**
 * @def   INODE_SIZE
 * @brief The size of i-node stored \b in \b the \b device.
 *
 * Note that this is the size of the struct in the device, \b NOT in memory.
 * The size in memory is larger because of some more members.
 */
#define	INODE_SIZE	32




 /**
 * @struct file_desc
 * @brief  File Descriptor
 */
struct file_desc {
	int		fd_mode;	/**< R or W */
	int		fd_pos;		/**< Current position for R/W. */
	struct inode*	fd_inode;	/**< Ptr to the i-node */
};




 #endif /* _ORANGES_FS_H_ */