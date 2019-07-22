;===============================================================================================
;File name:		kernel/syscall.asm
;Description:	*定义各系统调用
;Copyright:		Chauncey Zhang
;Date:		 	2019-7-16
;===============================================================================================


%include "const.inc"

INT_VECTOR_SYS_CALL equ     0x90    ;参考interruptor.c中的定义
ID_GET_TICKS        equ     0x0     ;sys_call_table[]中索引
ID_WRITE    	    equ     0x1

; 导入符号
extern  save
extern  sys_call_table
extern  p_proc_ready


[section .text]

; 导出符号
global  get_ticks
global  sys_call
global  write


; ===============================系统调用定义=====================================================
; 系统调用流程图:
; 系统调用(ring 1-3) -> 中断INT 90H -> 进入ring 0 -> sys_call() -> sys_call_table[]里面的sys_XXX() ------ return sys_call() -> return restart() -> 中断返回

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	;没有必要保存现场
	mov	eax, ID_GET_TICKS 	;给int 90h中断处理程序:系统调用传递参数.
	int	INT_VECTOR_SYS_CALL	;中断调用sys_call() 
	ret
;sys_get_ticks()定义在clock.c中


; ====================================================================================
;                          void write(char* buf, int len);
; ====================================================================================
write:
        mov     eax, ID_WRITE
        mov     ebx, [esp + 4]  ;将buf->ebx, len->ecx
        mov     ecx, [esp + 8]  ;syscall()中将这两个参数传给了sys_write()
        int     INT_VECTOR_SYS_CALL
        ret
;sys_write()定义在在tty.c中






; ==============================系统调用 OVER=====================================================


; ======================================================================
;                                 sys_call
;   =================================================================
;               系统调用int	INT_VECTOR_SYS_CALL中断服务程序
; ======================================================================
sys_call:
		;如果是ring1-3中断,则在PCB中保存低特权级环境(优先级切换时,ESP变成TSS.esp->PCB)
		;如果是ring0的中断重入,则在内核栈中save
        call    save	;切换到内核栈了

        sti

        push	dword [p_proc_ready]	;emmm, 我实在不想解释这里为什么push [XXX], 而不是push XXX;
                                        ;这是因为我之前提到过, 但是我想你大概忘了......
                                        ;在fxxk nasm中, 一切没有'[]'的标签&变量都是地址!
                                        ;所以直接push XXX,实际上是push address of XXX.
		push	ecx
		push	ebx
        ;实际上,对于大部分系统调用来说,上面三个参数根本就没有必要,之所以这么做,是为了代码简单;
        ;顺便提一下,这同是也反映了C Call Convention的好处(奥卡姆剃刀~)
		call    [sys_call_table + eax * 4]
		add	    esp, 4 * 3

        mov     [esi + EAXREG - P_STACKBASE], eax   ;procsss->EAX = eax

        cli

        ret		;由于save中push restart,所以这个ret返回到restart.
                ;restart将procss->EAX作返回值,并且中断返回iretd返回int	INT_VECTOR_SYS_CALL处

