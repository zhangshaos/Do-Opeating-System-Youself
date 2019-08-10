;================================================================================================
;File name:		kernel/syscall.asm
;Description:	*get_ticks()定义
;Copyright:		Chauncey Zhang
;Date:		 	2019-7-14
;Other:			参见<Orange's 一个操作系统的实现>
;===============================================================================================

%include "sconst.inc"

_NR_printx		equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_sendrec		equ 1
INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	printx
global	sendrec

bits 32
[section .text]

; ====================================================================================
;                  sendrec(int function, int src_dest, MESSAGE* msg);
; ====================================================================================
; Never call sendrec() directly, call send_recv() instead.
sendrec:
	push ebp	
	mov ebp, esp

	push ebx
	push ecx
	push edx

	mov	eax, _NR_sendrec
	mov	ebx, [ebp + 8]	; function
	mov	ecx, [ebp + 12]	; src_dest
	mov	edx, [ebp + 16]	; p_msg
	int	INT_VECTOR_SYS_CALL

	pop edx
	pop ecx
	pop ebx

	pop ebp
	ret

; ====================================================================================
;                          void printx(char* s);
; ====================================================================================
printx:
	push ebp
	mov ebp, esp

	push eax
	push edx

	mov	eax, _NR_printx
	mov	edx, [ebp + 8]
	int	INT_VECTOR_SYS_CALL

	pop edx
	pop eax

	pop ebp
	ret

