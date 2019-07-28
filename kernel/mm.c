

#include "type.h"
#include "struct_proc.h"
#include "global.h"
#include "func_proto.h"



/* 段选择符 -> 段基地址 */
inline u32 seg2phys(u16 seg)
{
	DESCRIPTOR* p_dest = &gdt[seg >> 3];

	return (p_dest->base_high << 24) | (p_dest->base_mid << 16) | (p_dest->base_low);
}


/* 基址 + 偏移 → 物理地址 */
inline u32 vir2phys(u32 seg_base, void *vir)
{
	return (u32)(((u32)seg_base) + (u32)(vir));
}

/** ======================= ldt_seg_linear =======================
 * <Ring 0~1> Calculate the linear address of a certain segment of a given
 * proc.
 * 
 * @param p   Whose (the proc ptr).
 * @param idx Which (one proc has more than one segments).
 * 
 * @return  The required linear address.
 */
inline int ldt_seg_linear(PROCESS* p, int idx)
{
	DESCRIPTOR * d = p->ldts + idx;
	return d->base_high << 24 | d->base_mid << 16 | d->base_low;
}


/** ====================== va2la =====================
 * <Ring 0~1> Virtual addr --> Linear addr.
 * 
 * @param pid  PID of the proc whose address is to be calculated.
 * @param va   Virtual address.
 * 
 * @return The linear address for the given virtual address.
 */
inline void* va2la(int pid, void* va)
{
	PROCESS* p = proc_table + pid;

 	u32 seg_base = ldt_seg_linear(p, INDEX_LDT_RW);
	u32 la = seg_base + (u32)va;

 	if (pid < NR_TASKS + NR_PROCS) {
		assert(la == (u32)va);
	}

 	return (void*)la;
}


