;================================================================================================
;File name:		kernel/syscall.asm
;Description:	*get_ticks()定义
;Copyright:		Chauncey Zhang
;Date:		 	2019-7-14
;Other:			参见<Orange's 一个操作系统的实现>
;===============================================================================================

%include "sconst.inc"

_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	get_ticks

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

