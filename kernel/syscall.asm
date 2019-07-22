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


; ====================================================================================
;                          void write(char* buf, int len);
; ====================================================================================
write:
        mov     eax, ID_WRITE
        mov     ebx, [esp + 4]
        mov     ecx, [esp + 8]
        int     INT_VECTOR_SYS_CALL
        ret







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

        push	dword [p_proc_ready]	;dword : 强制要 PROCESS 的前四个字节? gs?
		push	ecx
		push	ebx
		call    [sys_call_table + eax * 4]
		add	esp, 4 * 3

        mov     [esi + EAXREG - P_STACKBASE], eax   ;procsss->EAX = eax

        cli

        ret		;由于save中push restart,所以这个ret返回到restart.
                ;restart将procss->EAX作返回值,并且中断返回iretd返回int	INT_VECTOR_SYS_CALL处

