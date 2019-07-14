;================================================================================================
;File name:		lib/klia.c
;Description:	*底层函数库
;				*底层I/O函数
;Copyright:		Chauncey Zhang
;Date:		 	2019-6-29
;Other:			参见<Orange's 一个操作系统的实现>
;===============================================================================================

%include "sconst.inc" ;提供8259芯片的段鸥地址

; 导入全局变量
extern	disp_pos

[SECTION .text]

; 导出函数
global	disp_str
global	disp_color_str
global	out_byte
global	in_byte
global  enable_irq
global  disable_irq


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

; ========================================================================
;                  void disable_irq(int irq);
; ========================================================================
; Disable an interrupt request line by setting an 8259 bit.
; Equivalent code:
;	if(irq < 8){
;		out_byte(INT_M_CTLMASK, in_byte(INT_M_CTLMASK) | (1 << irq));
;	}
;	else{
;		out_byte(INT_S_CTLMASK, in_byte(INT_S_CTLMASK) | (1 << irq));
;	}
disable_irq:
		push 	ebp
		mov 	ebp, esp
		push	ecx						;保存现场

        mov     ecx, [ebp + 8]          ; irq
        pushf							; 压栈eflag
        cli
        mov     ah, 1
        rol     ah, cl                  ; ah = (1 << (irq % 8))
        cmp     cl, 8
        jae     disable_8               ; disable irq >= 8 at the slave 8259
disable_0:
        in      al, INT_M_CTLMASK
        test    al, ah
        jnz     dis_already             ; already disabled?
        or      al, ah
        out     INT_M_CTLMASK, al       ; set bit at master 8259
        popf
        mov     eax, 1                  ; disabled by this function

		pop ecx
        ret
disable_8:
        in      al, INT_S_CTLMASK
        test    al, ah
        jnz     dis_already             ; already disabled?
        or      al, ah
        out     INT_S_CTLMASK, al       ; set bit at slave 8259
        popf
        mov     eax, 1                  ; disabled by this function

		pop ecx
		pop ebp
        ret
dis_already:
        popf
        xor     eax, eax                ; already disabled

		pop ecx
		pop ebp
        ret

; ========================================================================
;                  void enable_irq(int irq);
; ========================================================================
; Enable an interrupt request line by clearing an 8259 bit.
; Equivalent code:
;       if(irq < 8){
;               out_byte(INT_M_CTLMASK, in_byte(INT_M_CTLMASK) & ~(1 << irq));
;       }
;       else{
;               out_byte(INT_S_CTLMASK, in_byte(INT_S_CTLMASK) & ~(1 << irq));
;       }
;
enable_irq:
		push 	ebp
		mov 	ebp, esp
		push	ecx						;保存现场
		push	eax

        mov     ecx, [ebp + 8]          ; irq
        pushf
        cli
        mov     ah, ~1
        rol     ah, cl                  ; ah = ~(1 << (irq % 8))
        cmp     cl, 8
        jae     enable_8                ; enable irq >= 8 at the slave 8259
enable_0:
        in      al, INT_M_CTLMASK
        and     al, ah
        out     INT_M_CTLMASK, al       ; clear bit at master 8259
        popf

		pop eax
		pop ecx
		pop ebp
        ret
enable_8:
        in      al, INT_S_CTLMASK
        and     al, ah
        out     INT_S_CTLMASK, al       ; clear bit at slave 8259
        popf

		pop eax
		pop ecx
		pop ebp
        ret


