;===============================================================================================
;File name:		kernel/syscall.asm
;Description:	*定义各系统调用
;Copyright:		Chauncey Zhang
;Date:		 	2019-7-16
;===============================================================================================


%include "const.inc"

INT_VECTOR_SYS_CALL equ     0x90    ;参考interruptor.c中的定义
ID_PRINTX           equ     0x0     ;sys_call_table[]中索引
ID_SENDRECV    	    equ     0x1

; 导入符号
extern  save
extern  sys_call_table
extern  p_proc_ready

bits 32
[section .text]

; 导出符号
global	sendrec
global	printx
global  sys_call


; ===============================系统调用定义=====================================================
; 系统调用流程图:
; 系统调用(ring 1-3) -> 中断INT 90H -> 进入ring 0 -> sys_call() -> sys_call_table[]里面的sys_XXX() ------ return sys_call() -> return restart() -> 中断返回

; ======================================================================================
;                  sendrec(int function, int src_dest, MESSAGE* msg);
; ======================================================================================
sendrec:
	mov	eax, ID_SENDRECV
	mov	ebx, [esp + 4]	; function
    mov	ecx, [esp + 8]	; src_dest
    mov	edx, [esp + 12]	; p_msg
    int	INT_VECTOR_SYS_CALL
	ret



; ====================================================================================
;                          void printx(char* s);
; ====================================================================================
printx:
    mov eax, ID_PRINTX
    mov	edx, [esp + 4]
    int	INT_VECTOR_SYS_CALL
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

        ;esi -> PCB, 防止下面的call [sys_call_table] 修改了esi
        push    esi

        ;传递参数(c调用惯例)
        ;func_name(ebx, ecx, edx, dword(p_proc_ready))
        push	dword [p_proc_ready]	;emmm, 我实在不想解释这里为什么push [XXX], 而不是push XXX;
                                        ;这是因为我之前提到过, 但是我想你大概忘了......
                                        ;在fxxk nasm中, 一切没有'[]'的标签&变量都是地址!
                                        ;所以直接push XXX,实际上是push address of XXX.

        push    edx
		push	ecx
		push	ebx
        ;实际上,对于大部分系统调用来说,上面三个参数根本就没有必要,之所以这么做,是为了代码简单;
        ;顺便提一下,这同是也反映了C Call Convention的好处(奥卡姆剃刀~)
		call    [sys_call_table + eax * 4]
		add	    esp, 4 * 4

        pop     esi
        mov     [esi + EAXREG - P_STACKBASE], eax   ;procsss->EAX = eax

        cli

        ret		;由于save中push restart,所以这个ret返回到restart.
                ;restart将procss->EAX作返回值,并且中断返回iretd返回int	INT_VECTOR_SYS_CALL处

