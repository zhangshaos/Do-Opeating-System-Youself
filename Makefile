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
LD_FLAGS		=	-s -m elf_i386 -Ttext $(ENTRYPOINT)


# 编译后文件
BOOT 	= 	boot/boot.bin boot/loader.bin
KERNEL 	= 	kernel.bin
#必须要将kernel.o放在ld第一个文件,否则kernel.o中的不会出现在.text段最前面(导致_start != .text(30400))
OBJS	= 	kernel/kernel.o\
			kernel/clock.o\
			kernel/console.o\
			kernel/exception_handler.o\
			kernel/global.o\
			kernel/init_idt.o\
			kernel/interrupt.o\
			kernel/keyboard.o\
			kernel/keymap.o\
			kernel/main.o\
			kernel/proc.o\
			kernel/syscall.o\
			kernel/tty.o\
			lib/klib.o\
			lib/kliba.o\
			lib/memory.o\
			lib/printf.o

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

kernel/console.o:	kernel/console.c include/const.h include/type.h include/struct_console.h include/struct_tty.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/exception_handler.o:	kernel/exception_handler.c include/const.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/global.o:	kernel/global.c include/const.h include/type.h include/struct_descript.h include/struct_proc.h include/const_interrupt.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/init_idt.o:	kernel/init_idt.c include/const.h include/const_interrupt.h include/global.h include/type.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/interrupt.o:	kernel/interrupt.asm include/const.inc
		nasm $< $(NASM_FLAGS_ELF) -o $@

kernel/kernel.o:	kernel/kernel.asm include/const.inc
		nasm $< $(NASM_FLAGS_ELF) -o $@

kernel/keyboard.o:	kernel/keyboard.c include/const.h include/struct_keyboard.h include/type.h include/struct_tty.h include/const_interrupt.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/keymap.o:	kernel/keymap.c include/const.h include/type.h include/struct_keyboard.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/main.o:	kernel/main.c include/type.h include/const.h include/global.h include/struct_descript.h include/const_interrupt.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/proc.o:	kernel/proc.c include/const.h include/struct_proc.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

kernel/syscall.o:	kernel/syscall.asm include/const.inc
		nasm $< $(NASM_FLAGS_ELF) -o $@

kernel/tty.o:	kernel/tty.c include/const.h include/type.h include/struct_tty.h include/struct_keyboard.h include/global.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

lib/klib.o:	lib/klib.c include/const.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

lib/kliba.o:lib/kliba.asm include/const.inc
		nasm $< $(NASM_FLAGS_ELF) -o $@

lib/memory.o:	lib/memory.asm
		nasm $< $(NASM_FLAGS_ELF) -o $@

lib/printf.o:	lib/printf.c include/type.h include/func_proto.h 
		gcc $< $(GCC_FLAGS) -o $@

