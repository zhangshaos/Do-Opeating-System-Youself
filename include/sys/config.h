/*************************************************************************//**
 *****************************************************************************
 * @file   config.h
 * @brief  
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/


/* 使用bximage时,指定了80m.img5是启动盘 */
#define	MINOR_BOOT			MINOR_hd2a

/**
 * boot parameters are stored by the loader, they should be
 * there when kernel is running and should not be overwritten
 * since kernel might use them at any time.
 */
#define	BOOT_PARAM_ADDR			0x900  /* physical address */
#define	BOOT_PARAM_MAGIC		0xB007 /* magic number */
#define	BI_MAG				    0
#define	BI_MEM_SIZE			    1
#define	BI_KERNEL_FILE			2
