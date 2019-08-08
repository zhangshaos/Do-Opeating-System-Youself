/*************************************************************************//**
 *****************************************************************************
 * @file   misc.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/


#include "func_proto.h"
#include "const.h"
#include "struct_fs.h"
#include "struct_hd.h"
#include "struct_proc.h"
#include "global.h"
#include "stdio.h"


/*****************************************************************************
 *                                search_file
 *****************************************************************************/
/**
 * Search the file and return the inode_nr.
 *
 * @param[in] path The full path of the file to search.
 * @return         Ptr to the i-node of the file if successful, otherwise zero.
 * 
 * @see open()
 * @see do_open()
 *****************************************************************************/
PUBLIC int search_file(char * path)
{
	char filename[MAX_PATH];
	memset(filename, 0, MAX_FILENAME_LEN);
	struct inode * dir_inode;	//由于我们采用的是flat file system,所以这个一般指向'/'iNode

	if (strip_path(filename, path, &dir_inode) != 0)
		return 0;

	if (filename[0] == 0)	/* path: "/" */
		return dir_inode->i_num;

	/**
	 * Search the dir for the file.
	 */
	int dir_blk0_nr = dir_inode->i_start_sect;	//目录iNode指向的第一个扇区
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;	//目录iNode包含的目录项(dir_entry)所占的扇区数
	// 目录iNode包含的最大目录项(dir_entry)数
	int nr_dir_entries =
	  					dir_inode->i_size / DIR_ENTRY_SIZE;/**
					       									* including unused slots
					      									* (the file has been deleted
					       									* but the slot is still there)
					       									*/
	// 已经搜索过的目录项(dir entry)数目,用于检测是否检查完'/'目录下的所有目录项
	int serched_dir_entries = 0;
	struct dir_entry * pde;
	// 搜索iNode'/'指向的包含目录项(dir entry)的所有扇区
	for (int i = 0; i < nr_dir_blks; i++) 
	{
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry *)fsbuf;
		// 搜索一个扇区中所有的目录项(dir entry)
		for (int j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) 
		{
			if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
				return pde->inode_nr;
			if (++serched_dir_entries > nr_dir_entries)
				break;
		}
		if (serched_dir_entries > nr_dir_entries) /* all entries have been iterated */
			break;
	}

	return 0;
}

/*****************************************************************************
 *                                strip_path
 *****************************************************************************/
/**
 * Get the basename from the fullpath.
 *
 * In Orange'S FS v1.0, all files are stored in the root directory.
 * There is no sub-folder thing.
 *
 * This routine should be called at the very beginning of file operations
 * such as open(), read() and write(). It accepts the full path and returns
 * two things: the basename and a ptr of the root dir's i-node.
 *
 * e.g. After stip_path(filename, "/blah", ppinode) finishes, we get:
 *      - filename: "blah"
 *      - *ppinode: root_inode
 *      - ret val:  0 (successful)
 *
 * Currently an acceptable pathname should begin with at most one `/'
 * preceding a filename.
 *
 * Filenames may contain any character except '/' and '\\0'.
 *
 * @param[out] filename The string for the result.
 * @param[in]  pathname The full pathname.
 * @param[out] ppinode  The ptr of the dir's inode will be stored here.
 * 
 * @return Zero if success, otherwise the pathname is not valid.
 *****************************************************************************/
PUBLIC int strip_path(char * filename, const char * pathname,
		      struct inode** ppinode)
{
	const char * s = pathname;
	char * t = filename;

	if (s == 0)
		return -1;

	if (*s == '/')
		s++;

	while (*s) {		/* check each character */
		if (*s == '/')
			return -1;
		*t++ = *s++;
		/* if filename is too long, just truncate it */
		if (t - filename >= MAX_FILENAME_LEN)
			break;
	}
	*t = 0;

	*ppinode = root_inode;

	return 0;
}


