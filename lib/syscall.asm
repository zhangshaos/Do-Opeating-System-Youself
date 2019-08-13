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
DEBUG_MEM_WRITE	equ 2
DEBUG_V_SPRINTF	equ	3
;DEBUG_BREAK_POINT	equ 4

INT_VECTOR_SYS_CALL equ 0x90

; 导出符号
global	printx
global	sendrec
global  DEBUG_MEMCPY
global  DEBUG_VSPRINTF
;global	BREAK_POINT

bits 32
[section .text]

; ====================================================================================
;                 <RING0-3> sendrec(int function, int src_dest, MESSAGE* msg);
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
;                         <RING0-3> void printx(char* s);
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




; DEBUG MODE USING!!!
; <RING =0-3> void DEBUG_MEMCPY(char *dest, char *source, int count);
DEBUG_MEMCPY:
	push	ebp
	mov 	ebp, esp

	push	ebx
	push 	ecx
	push	edx

	mov		eax, DEBUG_MEM_WRITE
	mov		ebx, [ebp + 8]
	mov		ecx, [ebp + 12]
	mov		edx, [ebp + 16]
	int		INT_VECTOR_SYS_CALL

	pop		edx
	pop		ecx
	pop		ebx

	pop		ebp
	ret

;<Ring0-3> int DEBUG_VSPRINTF(char *buf, const char *fmt, va_list args)
DEBUG_VSPRINTF:
	push	ebp
	mov		ebp,esp

	push	ebx
	push	ecx
	push	edx

	mov		eax,DEBUG_V_SPRINTF
	mov		ebx, [ebp + 8]
	mov		ecx, [ebp + 12]
	mov		edx, [ebp + 16]
	int		INT_VECTOR_SYS_CALL

	pop		edx
	pop		ecx
	pop		ebx

	pop		ebp
	ret
	

;<Ring0-3> void BREAK_POINT(void * eip)
;BREAK_POINT:
;	push ebp
;	mov ebp, esp

;	push eax
;	push edx

;	mov	eax, DEBUG_BREAK_POINT
;	mov	edx, [ebp + 8]
;	int	INT_VECTOR_SYS_CALL

;	pop edx
;	pop eax
;
;	pop ebp
;	ret