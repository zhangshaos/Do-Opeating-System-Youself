;===============================================================================================
;File name:		kernel/kernel.asm
;Description:	*内核
;Copyright:		Chauncey Zhang
;Date:		 	2019-7-16
;===============================================================================================


%include "const.inc"


; 导入全局变量
extern	gdt_ptr
extern	idt_ptr
extern	disp_pos


; 导入函数
extern	cstart
extern	kernel_main

; 导出符号
global StackTop ; 导出 kernel stack 栈顶, interrupt.asm-save()使用

;内核栈Kernel Stack
[SECTION .bss]
StackSpace		resb	2 * 1024
StackTop:		; 栈顶


;代码段
[section .text]	; 代码在此

global _start	; 导出 _start

;======================================kernel开始===========================================
_start:
	; 把 esp 从 LOADER 挪到 KERNEL
	mov	esp, StackTop	; 堆栈在 bss 段中

	mov	dword [disp_pos], 0

	sgdt	[gdt_ptr]	; cstart() 中将会用到 gdt_ptr
	call	cstart		; 在此函数中改变了gdt_ptr，让它指向新的GDT
	lgdt	[gdt_ptr]	; 使用新的GDT

	lidt	[idt_ptr]

	jmp	SELECTOR_KERNEL_CS:csinit
csinit:

	xor	eax, eax
	mov	ax, SELECTOR_TSS
	ltr	ax
	jmp	kernel_main
;========================================kernel结束==========================================

