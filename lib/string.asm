;================================================================================================
;File name:		lib/klib.c
;Description:	*部分函数库(非底层函数)
;Copyright:		Chauncey Zhang
;Date:		 	2019-6-29
;Other:			参见<Orange's 一个操作系统的实现>
;===============================================================================================
[SECTION .text]

; 导出函数
global	memcpy


; ===========================================================
; 	void* memcpy(void* es:p_dst, void* ds:p_src, int size);
; ===========================================================
memcpy:
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi
	push	ecx

	mov	edi, [ebp + 8]	; Destination
	mov	esi, [ebp + 12]	; Source
	mov	ecx, [ebp + 16]	; Counter
.1:
	cmp	ecx, 0		; 判断计数器
	jz	.2			; 计数器为零时跳出

	mov	al, [ds:esi]		; ┓
	inc	esi					; ┃
							; ┣ 逐字节移动
	mov	byte [es:edi], al	; ┃
	inc	edi					; ┛

	dec	ecx		; 计数器减一
	jmp	.1		; 循环
.2:
	mov	eax, [ebp + 8]	; 返回值

	pop	ecx
	pop	edi
	pop	esi
	pop	ebp

	ret			; 函数结束，返回