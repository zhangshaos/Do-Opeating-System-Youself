;================================================================================================
;File name:		kernel/syscall.asm
;Description:	*get_ticks()定义
;Copyright:		Chauncey Zhang
;Date:		 	2019-7-14
;Other:			参见<Orange's 一个操作系统的实现>
;===============================================================================================

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_write	    equ 1
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks
global	write

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	;没有必要保存现场
	mov	eax, _NR_get_ticks ;给int 90h中断处理程序:系统调用传递参数.
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================================
;                          void write(char* buf, int len);
; ====================================================================================
write:
        mov     eax, _NR_write
        mov     ebx, [esp + 4]
        mov     ecx, [esp + 8]
        int     INT_VECTOR_SYS_CALL
        ret
