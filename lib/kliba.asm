;================================================================================================
;File name:		lib/klib.c
;Description:	*底层函数库
;				*底层I/O函数
;Copyright:		Chauncey Zhang
;Date:		 	2019-6-29
;Other:			参见<Orange's 一个操作系统的实现>
;===============================================================================================

; 导入全局变量
extern	disp_pos

[SECTION .text]

; 导出函数
global	disp_str
global	disp_color_str
global	out_byte
global	in_byte

; ========================================================================
;                  void disp_str(char * pszInfo);
; ========================================================================
disp_str:
	push	ebp
	mov	ebp, esp
	push 	ebx
	push 	esi
	push 	edi
	push 	eax


	mov	esi, [ebp + 8]	; pszInfo
	mov	edi, [disp_pos]
	mov	ah, 0Fh
.1:
	lodsb
	test	al, al
	jz	.2
	cmp	al, 0Ah	; 是回车吗?
	jnz	.3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.1
.3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:
	mov	[disp_pos], edi

	pop eax
	pop edi
	pop esi
	pop ebx
	pop	ebp
	ret

; ========================================================================
;                  void disp_color_str(char * info, int color);
; ========================================================================
disp_color_str:
	push	ebp
	mov	ebp, esp
	push 	ebx
	push 	esi
	push 	edi
	push 	eax

	mov	esi, [ebp + 8]	; pszInfo
	mov	edi, [disp_pos]
	mov	ah, [ebp + 12]	; color
.1:
	lodsb
	test	al, al
	jz	.2
	cmp	al, 0Ah	; 是回车吗?
	jnz	.3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.1
.3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:
	mov	[disp_pos], edi

	pop eax
	pop edi
	pop esi
	pop ebx
	pop	ebp
	ret

; ========================================================================
;                  void out_byte(u16 port, u8 value);
; ========================================================================
out_byte:
	push ebp
	mov ebp,esp
	push edx
	push eax

	mov	edx, [ebp + 8]	; port
	mov	al,	 [ebp + 12]	; value
	out	dx, al
	nop	; 一点延迟
	nop

	pop eax
	pop edx
	pop ebp
	ret

; ========================================================================
;                  u8 in_byte(u16 port);
; ========================================================================
in_byte:
	push ebp
	mov ebp,esp
	push edx

	mov	edx, [ebp + 8]		; port
	xor	eax, eax
	in	al, dx
	nop	; 一点延迟
	nop

	pop ebx
	pop ebp
	ret

