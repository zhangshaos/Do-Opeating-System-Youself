;================================================================================================
;File name:		boot/loader.asm
;Description:	*作软盘的引导扇区,搜索根目录下的LOADER.BIN文件,并将其置于内存中,
;				*之后跳转到loader.
;Copyright:		Chauncey Zhang
;Date:		 	2019-6-28 
;Other:			参考<Orange's 一个操作系统的实现>
;===============================================================================================


org  0100h				;为Stack留下空间

	jmp	LABEL_START		; Start

%include	"fat12hdr.inc"	;定义描述fat12文件格式的变量
%include	"load.inc"		;(LOADER.BIN,KERNEL.BIN,页目录表和页表)在物理空间的位置
%include	"pm.inc"		;段描述符和门描述符的结构;	段描述符和选择子中的属性;	页目录项和页表项的属性


;====================================================GDT======================================================
;                                           段基址                段界限  属性
LABEL_GDT:				Descriptor             0,                    0, 0							; 空描述符
LABEL_DESC_FLAT_C:		Descriptor             0,              0fffffh, DA_CR  | DA_32 | DA_LIMIT_4K; 0 ~ 4G
LABEL_DESC_FLAT_RW:		Descriptor             0,              0fffffh, DA_DRW | DA_32 | DA_LIMIT_4K; 0 ~ 4G
LABEL_DESC_VIDEO:		Descriptor		 0B8000h,               0ffffh, DA_DRW | DA_DPL3			; 显存首地址
;====================================================GDT====================================================== 

GdtLen		equ	$ - LABEL_GDT
GdtPtr		dw	GdtLen - 1						; 段界限
			dd	LOADER_PHY_ADDR + LABEL_GDT		; 基地址 (让基地址八字节对齐将起到优化速度之效果，目前懒得改)
; The GDT is not a segment itself; instead, it is a data structure in linear address space.
; The base linear address and limit of the GDT must be loaded into the GDTR register. -- IA-32 Software Developer’s Manual, Vol.3A

;=========================GDT 选择子==============================
SelectorFlatC		equ	LABEL_DESC_FLAT_C	- LABEL_GDT
SelectorFlatRW		equ	LABEL_DESC_FLAT_RW	- LABEL_GDT
SelectorVideo		equ	LABEL_DESC_VIDEO	- LABEL_GDT + SA_RPL3
;=========================GDT 选择子==============================


BaseOfStack	equ	0100h

;==================================================开始=======================================================
LABEL_START:
	mov	ax, cs			;cs=BaseOfLoader=09000h(由boot的最后一条长跳转指令修改)
	mov	ds, ax
	mov	es, ax
	mov	ss, ax
	mov	sp, BaseOfStack

	mov	dh, 0					; 字符串索引->"Loading  "
	call	DispStrRealMode		; 显示字符串


;******************************得到内存数**********************************
	mov	ebx, 0			; ebx = 后续值, 开始时需为 0
	mov	di, _MemChkBuf	; es:di 指向一个地址范围描述符结构（Address Range Descriptor Structure）
.MemChkLoop:
	mov	eax, 0E820h		; eax = 0000E820h
	mov	ecx, 20			; ecx = 地址范围描述符结构的大小
	mov	edx, 0534D4150h	; edx = 'SMAP'
	int	15h				; int 15h
	jc	.MemChkFail
	add	di, 20
	inc	dword [_dwMCRNumber]	; dwMCRNumber = ARDS 的个数
	cmp	ebx, 0
	jne	.MemChkLoop
	jmp	.MemChkOK
.MemChkFail:
	mov	dword [_dwMCRNumber], 0
.MemChkOK:


;***********************下面在 A 盘的根目录寻找 KERNEL.BIN**********************
	mov	word [wSectorNo], SectorNoOfRootDirectory	
	xor	ah, ah	; ┓
	xor	dl, dl	; ┣ 软驱复位
	int	13h		; ┛
LABEL_SEARCH_IN_ROOT_DIR_BEGIN:
	cmp	word [wRootDirSizeForLoop], 0	; ┓
	jz	LABEL_NO_KERNELBIN				; ┣ 判断根目录区是不是已经读完, 如果读完表示没有找到 KERNEL.BIN
	dec	word [wRootDirSizeForLoop]		; ┛
	mov	ax, KERNEL_FILE_SEG
	mov	es, ax				; es <- KERNEL_FILE_SEG
	mov	bx, KERNEL_FILE_OFF	; bx <- KERNEL_FILE_OFF	于是, es:bx = KERNEL_FILE_SEG:KERNEL_FILE_OFF = KERNEL_FILE_SEG * 10h + KERNEL_FILE_OFF
	mov	ax, [wSectorNo]		; ax <- Root Directory 中的某 Sector 号
	mov	cl, 1
	call	ReadSector			; 将ax扇区读入es:dx内存中

	mov	si, KernelFileName	; ds:si -> "KERNEL  BIN"
	mov	di, KERNEL_FILE_OFF	; es:di -> KERNEL_FILE_SEG:???? = KERNEL_FILE_SEG*10h+????
	cld
	mov	dx, 10h
LABEL_SEARCH_FOR_KERNELBIN:
	cmp	dx, 0							  ;┓
	jz	LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR;┣ 循环次数控制, 如果已经读完了一个 Sector, 就跳到下一个 Sector
	dec	dx								  ;┛
	mov	cx, 11
LABEL_CMP_FILENAME:
	cmp	cx, 0				;┓
	jz	LABEL_FILENAME_FOUND;┣ 循环次数控制, 如果比较了 11 个字符都相等, 表示找到
	dec	cx					;┛
	lodsb				; ds:si -> al
	cmp	al, byte [es:di]; if al == es:di,比较字符串
	jz	LABEL_GO_ON
	jmp	LABEL_DIFFERENT
LABEL_GO_ON:
	inc	di
	jmp	LABEL_CMP_FILENAME	;	继续循环

LABEL_DIFFERENT:		;转向下一个目录项
	and	di, 0FFE0h		; else	   ┓这时di的值不知道是什么, di &= e0 为了让它是 20h 的倍数
	add	di, 20h			;     	   ┃
	mov	si, KernelFileName	; 	   ┣ di += 20h 下一个目录条目
	jmp	LABEL_SEARCH_FOR_KERNELBIN;┛

LABEL_GOTO_NEXT_SECTOR_IN_ROOT_DIR:
	add	word [wSectorNo], 1
	jmp	LABEL_SEARCH_IN_ROOT_DIR_BEGIN

LABEL_NO_KERNELBIN:
	mov	dh, 3			        ; "No KERNEL."
	call	DispStrRealMode		; 显示字符串
	jmp	$						; 没有找到 KERNEL.BIN, 死循环在这里


;*************************找到 KERNEL.BIN 后便来到这里继续*****************************
LABEL_FILENAME_FOUND:
	mov	ax, RootDirSectors
	and	di, 0FFF0h		; di -> 当前条目的开始

	push eax
	mov	eax, [es : di + 01Ch]		; ┓
	mov	dword [dwKernelSize], eax	; ┛保存 KERNEL.BIN 文件大小
	cmp	eax, KERNEL_VALID_SPACE
	ja	.1
	pop	eax
	jmp	.2
.1:
	mov	dh, 4				; "Too Large"
	call	DispStrRealMode	; 显示字符串
	jmp	$					; KERNEL.BIN 太大，死循环在这里
.2:
	add	di, 01Ah		; di -> 首 Sector
	mov	cx, word [es:di]
	push	cx			; 保存此 Sector 在 FAT 中的序号
	add	cx, ax
	add	cx, DeltaSectorNo	; 这时 cl 里面是 LOADER.BIN 的起始扇区号 (从 0 开始数的序号)
	mov	ax, KERNEL_FILE_SEG
	mov	es, ax				; es <- KERNEL_FILE_SEG
	mov	bx, KERNEL_FILE_OFF	; bx <- KERNEL_FILE_OFF	于是, es:bx = KERNEL_FILE_SEG:KERNEL_FILE_OFF = KERNEL_FILE_SEG * 10h + KERNEL_FILE_OFF
	mov	ax, cx				; ax <- Sector 号

LABEL_GOON_LOADING_FILE:
	push	ax			; ┓
	push	bx			; ┃
	mov	ah, 0Eh			; ┃ 每读一个扇区就在 "Loading " 后面打一个点, 形成这样的效果:
	mov	al, '.'			; ┃
	mov	bl, 0Fh			; ┃ Loading ......
	int	10h			    ; ┃
	pop	bx			    ; ┃
	pop	ax			    ; ┛

	mov	cl, 1
	call	ReadSector
	pop	ax				; 取出此 Sector 在 FAT 中的序号
	call	GetFATEntry
	cmp	ax, 0FFFh
	jz	LABEL_FILE_LOADED
	push	ax			; 保存 Sector 在 FAT 中的序号
	mov	dx, RootDirSectors
	add	ax, dx
	add	ax, DeltaSectorNo
	add	bx, [BPB_BytsPerSec]
	jc	.1			; 如果 bx 重新变成 0，说明内核大于 64KB
	jmp	.2
.1:
	push	ax			; es += 0x1000  ← es 指向下一个段
	mov	ax, es
	add	ax, 1000h
	mov	es, ax
	pop	ax
.2:
	jmp	LABEL_GOON_LOADING_FILE
LABEL_FILE_LOADED:

	call	KillMotor		; 关闭软驱马达

	mov	dh, 2			    ; "Ready."
	call	DispStrRealMode	; 显示字符串
	
;***************************下面准备跳入保护模式**************************

; 加载 GDTR
	lgdt	[GdtPtr]

; 关中断
	cli

; 打开地址线A20
	in	al, 92h
	or	al, 00000010b
	out	92h, al

; 准备切换到保护模式
	mov	eax, cr0
	or	eax, 1
	mov	cr0, eax

; 真正进入保护模式
	jmp	dword SelectorFlatC:(LOADER_PHY_ADDR+LABEL_PM_START)	;16->32位代码段的神奇跳转.为了确保offset是32位,必须加上dword强制转换


;==================================变量==========================================
wRootDirSizeForLoop	dw	RootDirSectors	; Root Directory 占用的扇区数
wSectorNo		    dw	0		; 要读取的扇区号
bOdd			    db	0		; 奇数还是偶数
dwKernelSize		dd	0		; KERNEL.BIN 文件大小

;===================================字符串=======================================
KernelFileName		db	"KERNEL  BIN", 0	; KERNEL.BIN 之文件名
; 为简化代码, 下面每个字符串的长度均为 MessageLength
MessageLength		equ	9
LoadMessage:		db	"Loading  "
Message1			db	"         "
Message2			db	"Ready.   "
Message3			db	"No KERNEL"
Message4			db	"Too Large"

;====================================函数========================================
;*******************************************************************************
; 							函数名: DispStrRealMode								*
;*******************************************************************************
; 运行环境:
;	实模式（保护模式下显示字符串由函数 DispStr 完成）
; 作用:
;	显示一个字符串, 函数开始时 dh 中应该是字符串序号(0-based)
DispStrRealMode:
	mov	ax, MessageLength
	mul	dh
	add	ax, LoadMessage
	mov	bp, ax			; ┓
	mov	ax, ds			; ┣ ES:BP = 串地址
	mov	es, ax			; ┛
	mov	cx, MessageLength	; CX = 串长度
	mov	ax, 01301h			; AH = 13,  AL = 01h
	mov	bx, 0007h			; 页号为0(BH = 0) 黑底白字(BL = 07h)
	mov	dl, 0
	add	dh, 3				; 从第 3 行往下显示
	int	10h					; int 10h
	ret

;***********************************************************************
; 							函数名: ReadSector							*
;***********************************************************************
; 作用:
;	从序号(Directory Entry 中的 Sector 号)为 ax 的的 Sector 开始, 将 cl 个 Sector 读入 es:bx 中
ReadSector:
	; -----------------------------------------------------------------------
	; 怎样由扇区号求扇区在磁盘中的位置 (扇区号 -> 柱面号, 起始扇区, 磁头号)
	; -----------------------------------------------------------------------
	; 设扇区号为 x
	;                          ┌ 柱面号 = y >> 1
	;       x           ┌ 商 y ┤
	; -------------- => ┤      └ 磁头号 = y & 1
	;  每磁道扇区数       │
	;                   └ 余 z => 起始扇区号 = z + 1
	push	bp
	mov	bp, sp
	sub	esp, 2			; 辟出两个字节的堆栈区域保存要读的扇区数: byte [bp-2]

	mov	byte [bp-2], cl
	push	bx				; 保存 bx
	mov	bl, [BPB_SecPerTrk]	; bl: 除数
	div	bl					; y 在 al 中, z 在 ah 中
	inc	ah					; z ++
	mov	cl, ah			; cl <- 起始扇区号
	mov	dh, al			; dh <- y
	shr	al, 1			; y >> 1 (其实是 y/BPB_NumHeads, 这里BPB_NumHeads=2)
	mov	ch, al			; ch <- 柱面号
	and	dh, 1			; dh & 1 = 磁头号
	pop	bx				; 恢复 bx
	; 至此, "柱面号, 起始扇区, 磁头号" 全部得到 ^^^^^^^^^^^^^^^^^^^^^^^^
	mov	dl, [BS_DrvNum]		; 驱动器号 (0 表示 A 盘)
.GoOnReading:
	mov	ah, 2				; 读
	mov	al, byte [bp-2]		; 读 al 个扇区
	int	13h
	jc	.GoOnReading		; 如果读取错误 CF 会被置为 1, 这时就不停地读, 直到正确为止

	add	esp, 2
	pop	bp

	ret

;***********************************************************************
; 							函数名: GetFATEntry							*
;***********************************************************************
; 作用:
;	找到序号为 ax 的 Sector 在 FAT 中的条目, 结果放在 ax 中
;	需要注意的是, 中间需要读 FAT 的扇区到 es:bx 处, 所以函数一开始保存了 es 和 bx
GetFATEntry:
	push	es
	push	bx
	push	ax
	mov	ax, KERNEL_FILE_SEG	; ┓
	sub	ax, 0100h			; ┣ 在 KERNEL_FILE_SEG 后面留出 4K 空间用于存放 FAT
	mov	es, ax				; ┛
	pop	ax
	mov	byte [bOdd], 0
	mov	bx, 3
	mul	bx			; dx:ax = ax * 3
	mov	bx, 2
	div	bx			; dx:ax / 2  ==>  ax <- 商, dx <- 余数
	cmp	dx, 0
	jz	LABEL_EVEN
	mov	byte [bOdd], 1
LABEL_EVEN:;偶数
	xor	dx, dx				; 现在 ax 中是 FATEntry 在 FAT 中的偏移量. 下面来计算 FATEntry 在哪个扇区中(FAT占用不止一个扇区)
	mov	bx, [BPB_BytsPerSec]
	div	bx					; dx:ax / BPB_BytsPerSec  ==>	ax <- 商   (FATEntry 所在的扇区相对于 FAT 来说的扇区号)
							; dx <- 余数 (FATEntry 在扇区内的偏移)。
	push	dx
	mov	bx, 0				; bx <- 0	于是, es:bx = (KERNEL_FILE_SEG - 100):00 = (KERNEL_FILE_SEG - 100) * 10h
	add	ax, SectorNoOfFAT1	; 此句执行之后的 ax 就是 FATEntry 所在的扇区号
	mov	cl, 2
	call	ReadSector		; 读取 FATEntry 所在的扇区, 一次读两个, 避免在边界发生错误, 因为一个 FATEntry 可能跨越两个扇区
	pop	dx
	add	bx, dx
	mov	ax, [es:bx]
	cmp	byte [bOdd], 1
	jnz	LABEL_EVEN_2
	shr	ax, 4
LABEL_EVEN_2:
	and	ax, 0FFFh

LABEL_GET_FAT_ENRY_OK:
	pop	bx
	pop	es
	ret

;***************************************************************************
; 							函数名: KillMotor								*
;***************************************************************************
; 作用:
;	关闭软驱马达
KillMotor:
	push	dx
	mov	dx, 03F2h
	mov	al, 0
	out	dx, al
	pop	dx
	ret



;=================================================进入保护模式===================================================
[SECTION .s32]
ALIGN	32
[BITS	32]

LABEL_PM_START:
    ;cs已经被跳入此段的长跳转修改了,此处不必修改.
	mov	ax, SelectorVideo
	mov	gs, ax
	mov	ax, SelectorFlatRW
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	ss, ax
	mov	esp, TopOfStack

	push	szMemChkTitle
	call	DispStr
	add	esp, 4

	call	DispMemInfo
	call	SetupPaging

	call	InitKernel


	mov	dword [BOOT_PARAM_ADDR], BOOT_PARAM_MAGIC	; BootParam[0] = BootParamMagic;
	mov	eax, [dwMemSize]							;
	mov	[BOOT_PARAM_ADDR + 4], eax					; BootParam[1] = MemSize;
	mov	eax, KERNEL_FILE_SEG
	shl	eax, 4
	add	eax, KERNEL_FILE_OFF
	mov	[BOOT_PARAM_ADDR + 8], eax					; BootParam[2] = KernelFilePhyAddr;
	

	jmp SelectorFlatC:KRNL_ENT_PT_PHY_ADDR	; [正式进入内核]
;=====================================================END======================================================
	


;===============================================函数===============================================
;*******************************************************
;					显示 AL 中的数字					 *
;*******************************************************
DispAL:
	push	ecx
	push	edx
	push	edi

	mov	edi, [dwDispPos]

	mov	ah, 0Fh			; 0000b: 黑底    1111b: 白字
	mov	dl, al
	shr	al, 4
	mov	ecx, 2
.begin:
	and	al, 01111b
	cmp	al, 9
	ja	.1
	add	al, '0'
	jmp	.2
.1:
	sub	al, 0Ah
	add	al, 'A'
.2:
	mov	[gs:edi], ax
	add	edi, 2

	mov	al, dl
	loop	.begin
	;add	edi, 2

	mov	[dwDispPos], edi

	pop	edi
	pop	edx
	pop	ecx

	ret

;**********************************************************
; 						显示一个整形数						*
;**********************************************************
DispInt:
	mov	eax, [esp + 4]
	shr	eax, 24
	call	DispAL

	mov	eax, [esp + 4]
	shr	eax, 16
	call	DispAL

	mov	eax, [esp + 4]
	shr	eax, 8
	call	DispAL

	mov	eax, [esp + 4]
	call	DispAL

	mov	ah, 07h			; 0000b: 黑底    0111b: 灰字
	mov	al, 'h'
	push	edi
	mov	edi, [dwDispPos]
	mov	[gs:edi], ax
	add	edi, 4
	mov	[dwDispPos], edi
	pop	edi

	ret

;******************************************************
; 					显示一个字符串					   	*
;******************************************************
DispStr:
	push	ebp
	mov	ebp, esp
	push	ebx
	push	esi
	push	edi

	mov	esi, [ebp + 8]	; pszInfo
	mov	edi, [dwDispPos]
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
	mov	[dwDispPos], edi

	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

;*****************************************************
; 						换行						  *
;*****************************************************
DispReturn:
	push	szReturn
	call	DispStr			;printf("\n");
	add	esp, 4

	ret


;***********************************************************
; 					内存拷贝，仿 memcpy						 *
;														   *
; void* MemCpy(void* es:pDest, void* ds:pSrc, int iSize);  *
;***********************************************************
MemCpy:
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
	jz	.2		; 计数器为零时跳出

	mov	al, [ds:esi]		; ┓
	inc	esi			        ; ┃
					        ; ┣ 逐字节移动
	mov	byte [es:edi], al	; ┃
	inc	edi			        ; ┛

	dec	ecx		; 计数器减一
	jmp	.1		; 循环
.2:
	mov	eax, [ebp + 8]	; 返回值

	pop	ecx
	pop	edi
	pop	esi
	pop	ebp

	ret			; 函数结束，返回


;*************************************************************
; 						显示内存信息						   *
;*************************************************************
DispMemInfo:
	push	esi
	push	edi
	push	ecx

	mov	esi, MemChkBuf
	mov	ecx, [dwMCRNumber];for(int i=0;i<[MCRNumber];i++)	// 每次得到一个ARDS(Address Range Descriptor Structure)结构
.loop:					  ;{
	mov	edx, 5				;for(int j=0;j<5;j++)			// 每次得到一个ARDS中的成员，共5个成员
	mov	edi, ARDStruct		;{								// 依次显示：BaseAddrLow，BaseAddrHigh，LengthLow，LengthHigh，Type
.1:	
	push	dword [esi]
	call	DispInt			;	DispInt(MemChkBuf[j*4]); 	// 显示一个成员
	pop	eax	
	stosd					;	ARDStruct[j*4] = MemChkBuf[j*4];
	add	esi, 4		
	dec	edx			
	cmp	edx, 0		
	jnz	.1					;}
	call	DispReturn		;printf("\n");
	cmp	dword [dwType], 1	;if(Type == AddressRangeMemory) // AddressRangeMemory : 1; AddressRangeReserved : 2
	jne	.2					;{
	mov	eax, [dwBaseAddrLow]
	add	eax, [dwLengthLow]
	cmp	eax, [dwMemSize]	;	if(BaseAddrLow + LengthLow > MemSize)
	jb	.2
	mov	[dwMemSize], eax	;		MemSize = BaseAddrLow + LengthLow;
.2:							;}
	loop	.loop		  ;}

	call	DispReturn		;printf("\n");
	push	szRAMSize
	call	DispStr			;printf("RAM size:");
	add	esp, 4

	push	dword [dwMemSize]
	call	DispInt			;DispInt(MemSize);
	add	esp, 4

	pop	ecx
	pop	edi
	pop	esi
	ret


;***************************************************
; 					启动分页机制					 *
;*************************************************** 
SetupPaging:
	; 根据内存大小计算应初始化多少PDE以及多少页表
	xor	edx, edx
	mov	eax, [dwMemSize]
	mov	ebx, 400000h; 400000h = 4M = 4096 * 1024, 一个页表对应的内存大小
	div	ebx
	mov	ecx, eax	; 此时 ecx 为页表的个数，也即 PDE 应该的个数
	test	edx, edx
	jz	.no_remainder
	inc	ecx			; 如果余数不为 0 就需增加一个页表
.no_remainder:
	push	ecx		; 暂存页表个数

	; 为简化处理, 所有线性地址对应相等的物理地址. 并且不考虑内存空洞.

	; 首先初始化页目录
	mov	ax, SelectorFlatRW
	mov	es, ax
	mov	edi, PAGE_DIR_BASE	; 此段首地址为 PAGE_DIR_BASE
	xor	eax, eax
	mov	eax, PAGE_TBL_BASE | PG_P  | PG_USU | PG_RWW
.1:
	stosd
	add	eax, 4096		; 为了简化, 所有页表在内存中是连续的.
	loop	.1

	; 再初始化所有页表
	pop	eax				; 页表个数
	mov	ebx, 1024		; 每个页表 1024 个 PTE
	mul	ebx
	mov	ecx, eax		; PTE个数 = 页表个数 * 1024
	mov	edi, PAGE_TBL_BASE	; 此段首地址为 PAGE_TBL_BASE
	xor	eax, eax
	mov	eax, PG_P  | PG_USU | PG_RWW
.2:
	stosd
	add	eax, 4096		; 每一页指向 4K 的空间
	loop	.2

	mov	eax, PAGE_DIR_BASE
	mov	cr3, eax
	mov	eax, cr0
	or	eax, 80000000h
	mov	cr0, eax
	jmp	short .3
.3:
	nop
	ret


;****************************************************
; 					InitKernel						*
;													* 
;	将KERNEL.BIN的内容按照ELF文件格式对齐后放到新的位置	  *
;****************************************************
InitKernel:	; 遍历每一个 Program Header，根据 Program Header 中的信息来确定把什么放进内存，放到什么位置，以及放多少。
	;注意此时ds=SelectorFlatRW(基地址是0)
    xor	esi, esi
	mov	cx, word [KERNEL_FILE_PHY_ADDR + 2Ch]	; ┓ ecx <- pELFHdr->e_phnum
	movzx	ecx, cx								; ┛
	mov	esi, [KERNEL_FILE_PHY_ADDR + 1Ch]		; esi <- pELFHdr->e_phoff
	add	esi, KERNEL_FILE_PHY_ADDR				; esi <- OffsetOfKernel + pELFHdr->e_phoff
.Begin:
	mov	eax, [esi + 0]
	cmp	eax, 0  ; PT_NULL(ELF格式中Program Header的p_type属性,描述当前段的类型)
	jz	.NoAction
	push	dword [esi + 010h]		; size	┓
	mov	eax, [esi + 04h]		        ;	┃
	add	eax, KERNEL_FILE_PHY_ADDR		;	┣ ::memcpy(	(void*)(pPHdr->p_vaddr),
	push	eax			        	; src	┃		uchCode + pPHdr->p_offset,
	push	dword [esi + 08h]		; dst	┃		pPHdr->p_filesz;
	call	MemCpy			        	;	┃
	add	esp, 12				            ;	┛
.NoAction:
	add	esi, 020h			; esi += pELFHdr->e_phentsize
	dec	ecx
	jnz	.Begin

	ret



;============================================数据段==========================================
[SECTION .data1]
ALIGN	32

LABEL_DATA:
; 实模式下使用这些符号
;***********************字符串********************
_szMemChkTitle:			db	"BaseAddrL BaseAddrH LengthLow LengthHigh   Type", 0Ah, 0
_szRAMSize:				db	"RAM size: ", 0
; _szCylinder			db	"HD Info : C=", 0
; _szHead				db	" H=", 0
; _szSector				db	" S=", 0
; _szNOHD				db	"No hard drive. System halt.", 0
_szReturn:				db	0Ah, 0


;***********************变量*********************
; _dwNrCylinder		dd	0
; _dwNrHead			dd	0
; _dwNrSector		dd	0
_dwMCRNumber:		dd	0	; Memory Check Result
_dwDispPos:			dd	(80 * 6 + 0) * 2	; 屏幕第 6 行, 第 0 列。
_dwMemSize:			dd	0
_ARDStruct:			; Address Range Descriptor Structure
_dwBaseAddrLow:		dd	0
_dwBaseAddrHigh:	dd	0
_dwLengthLow:		dd	0
_dwLengthHigh:		dd	0
_dwType:		    dd	0
_MemChkBuf:	times	256	db	0


; 保护模式下使用这些符号
szMemChkTitle	equ	LOADER_PHY_ADDR + _szMemChkTitle
szRAMSize		equ	LOADER_PHY_ADDR + _szRAMSize
; szCylinder	equ	LOADER_PHY_ADDR + _szCylinder
; szHead		equ	LOADER_PHY_ADDR + _szHead
; szSector		equ	LOADER_PHY_ADDR + _szSector
; szNOHD		equ	LOADER_PHY_ADDR + _szNOHD
szReturn		equ	LOADER_PHY_ADDR + _szReturn
; dwNrCylinder	equ	LOADER_PHY_ADDR + _dwNrCylinder
; dwNrHead		equ	LOADER_PHY_ADDR + _dwNrHead
; dwNrSector	equ	LOADER_PHY_ADDR + _dwNrSector
dwDispPos		equ	LOADER_PHY_ADDR + _dwDispPos
dwMemSize		equ	LOADER_PHY_ADDR + _dwMemSize
dwMCRNumber		equ	LOADER_PHY_ADDR + _dwMCRNumber
ARDStruct		equ	LOADER_PHY_ADDR + _ARDStruct
dwBaseAddrLow	equ	LOADER_PHY_ADDR + _dwBaseAddrLow
dwBaseAddrHigh	equ	LOADER_PHY_ADDR + _dwBaseAddrHigh
dwLengthLow		equ	LOADER_PHY_ADDR + _dwLengthLow
dwLengthHigh	equ	LOADER_PHY_ADDR + _dwLengthHigh
dwType			equ	LOADER_PHY_ADDR + _dwType
MemChkBuf		equ	LOADER_PHY_ADDR + _MemChkBuf


;********************保护模式下,堆栈***********************
StackSpace:	times	1000h	db	0
TopOfStack					equ	LOADER_PHY_ADDR + $	; 栈顶

