/*================================================================================================
File name:		include/string.h
Description:	*操作内存函数的原型
Copyright:		Chauncey Zhang
Date:		 	2019-6-29
Other:			参见<Orange's 一个操作系统的实现>
===============================================================================================*/

PUBLIC	void*	memcpy(void* p_dst, void* p_src, int size);
PUBLIC	void	memset(void* p_dst, char ch, int size);
PUBLIC  char*   strcpy(char* p_dst, const char* p_src);
PUBLIC	int	    strlen(const char* p_str);
PUBLIC	int	    memcmp(const void * s1, const void *s2, int n);
PUBLIC	int	    strcmp(const char * s1, const char *s2);
PUBLIC	char*	strcat(char * s1, const char *s2);

/**
 * `phys_copy' and `phys_set' are used only in the kernel, where segments
 * are all flat (based on 0). In the meanwhile, currently linear address
 * space is mapped to the identical physical address space. Therefore,
 * a `physical copy' will be as same as a common copy, so does `phys_set'.
 */
#define	phys_copy	memcpy
#define	phys_set	memset

