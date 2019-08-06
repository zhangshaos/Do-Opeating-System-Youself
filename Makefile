# Entry point
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
ENTRYPOINT	= 0x30400


# 编译,链接
NASM_FLAGS_BIN 	=	-I boot/include/
NASM_FLAGS_ELF	=	-I include/ -f elf
# -m32					:	long,pointer视为32位
# -fno_stack-protector	:	禁止堆栈保护(节省空间?)
# -fno-builtin			;	禁止内部函数, 防止自定义的函数与內建函数重名
GCC_FLAGS		=	-I include/ -c -m32 -fno-stack-protector -fno-builtin
# -m 		: 	模拟环境
# -Ttext 	:	相当与org汇编指令
# -s 		:	清除符号信息
# -Map 		: 	....
LD_FLAGS		=	-s -m elf_i386 -Ttext $(ENTRYPOINT) -Map ld.map


# 编译后文件
BOOT 	= 	boot/boot.bin boot/loader.bin
KERNEL 	= 	kernel.bin
#必须要将kernel.o放在ld第一个文件,否则kernel.o中的不会出现在.text段最前面(导致_start != .text(30400))
OBJS	= 	kernel/kernel.o\
			kernel/clock.o\
			kernel/exception_handler.o\
			kernel/global.o\
			kernel/init_idt.o\
			kernel/interrupt.o\
			kernel/ipc.o\
			kernel/main.o\
			kernel/proc.o\
			sys/syscall.o\
			sys/syscallc.o\
			sys/systask.o\
			lib/klib.o\
			lib/kliba.o\
			lib/memory.o\
			lib/misc.o\
			lib/printf.o\
			lib/open.o\
			lib/close.o\
			fs/fs_main.o\
			fs/fs_misc.o\
			fs/fs_open.o\
			hd/hd.o\
			mm/mm.o\
			tty/console.o\
			tty/keyboard.o\
			tty/keymap.o\
			tty/tty.o


# 动作
.PHONY : nop image clean mk_boot mk_obj mk_kernel mk_image
nop :
		@echo "make image : 编译链接所有文件,并写入a.img中\nmake clean : 删除所有生成的文件"

image :	clean mk_boot mk_obj mk_kernel mk_image 

clean :	
		rm -f  $(OBJS) $(BOOT)

mk_boot	:	$(BOOT)

mk_obj	:	$(OBJS)

mk_kernel:	$(KERNEL)

mk_image:	$(BOOT) $(KERNEL)
		dd if=boot/boot.bin of=a.img bs=512 count=1 conv=notrunc
		sudo mount -o loop a.img /mnt/floppy/
		sudo cp -fv boot/loader.bin /mnt/floppy/
		sudo cp -fv kernel.bin /mnt/floppy
		sudo umount /mnt/floppy


# 中间文件
boot/boot.bin:	boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
		nasm $(NASM_FLAGS_BIN) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/load.inc boot/include/fat12hdr.inc boot/include/pm.inc
		nasm $(NASM_FLAGS_BIN) -o $@ $<

$(KERNEL):		$(OBJS)
		ld $(OBJS) $(LD_FLAGS) -o $@ 


kernel/clock.o:	kernel/clock.c include/const.h include/global.h include/func_proto.h include/const_interrupt.h 
		gcc $< $(GCC_FLAGS) -o $@

tty/console.o:	tty/console.c include/const.h include/type.h include/struct_console.h include/struct_tty.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/exception_handler.o:	kernel/exception_handler.c include/const.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/global.o:	kernel/global.c include/const.h include/type.h include/struct_descript.h include/struct_proc.h include/const_interrupt.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

hd/hd.o:	hd/hd.c #还有其他的头文件暂时不管了...
		gcc $< $(GCC_FLAGS) -o $@

kernel/init_idt.o:	kernel/init_idt.c include/const.h include/const_interrupt.h include/global.h include/type.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/interrupt.o:	kernel/interrupt.asm include/const.inc
		nasm $< $(NASM_FLAGS_ELF) -o $@

kernel/ipc.o:	kernel/ipc.c include/type.h include/const.h include/struct_proc.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/kernel.o:	kernel/kernel.asm include/const.inc
		nasm $< $(NASM_FLAGS_ELF) -o $@

tty/keyboard.o:	tty/keyboard.c include/const.h include/struct_keyboard.h include/type.h include/struct_tty.h include/const_interrupt.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

tty/keymap.o:	tty/keymap.c include/const.h include/type.h include/struct_keyboard.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/main.o:	kernel/main.c include/type.h include/const.h include/global.h include/struct_descript.h include/const_interrupt.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

mm/mm.o:	mm/mm.c include/type.h include/struct_proc.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/proc.o:	kernel/proc.c include/const.h include/struct_proc.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

sys/syscall.o:	sys/syscall.asm include/const.inc
		nasm $< $(NASM_FLAGS_ELF) -o $@

sys/syscallc.o:	sys/syscallc.c include/type.h include/const.h include/struct_proc.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

sys/systask.o:	sys/systask.c include/const.h include/type.h include/struct_proc.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

tty/tty.o:	tty/tty.c include/const.h include/type.h include/struct_tty.h include/struct_keyboard.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

lib/klib.o:	lib/klib.c include/const.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

lib/kliba.o:lib/kliba.asm include/const.inc
		nasm $< $(NASM_FLAGS_ELF) -o $@

lib/memory.o:	lib/memory.asm
		nasm $< $(NASM_FLAGS_ELF) -o $@

lib/misc.o:	lib/misc.c include/const.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

lib/printf.o:	lib/printf.c include/type.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

lib/open.o:	lib/open.c 
		gcc $< $(GCC_FLAGS) -o $@

lib/close.o:	lib/close.c 
		gcc $< $(GCC_FLAGS) -o $@

fs/fs_main.o:	fs/fs_main.c 
		gcc $< $(GCC_FLAGS) -o $@

fs/fs_misc.o:	fs/fs_misc.c 
		gcc $< $(GCC_FLAGS) -o $@

fs/fs_open.o:	fs/fs_open.c 
		gcc $< $(GCC_FLAGS) -o $@
