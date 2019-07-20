>*This is my own toy-operating-system when reading the book named 《Oranges' 一个操作系统的实现》*

# Do-Opeating-System-Youself

## Developer environment:

* Ubuntu 18.04.2 LTS

* NASM version 2.13.02

* gcc 7.4.0

* Bochs x86 Emulator 2.6.9 (source version with debugger)

  **WARNING** : *Bochs must to be source version where you can debug your own OS.*




## Dir strucure:
* boot/ : 软盘引导-boot.asm; 内核引导-loader.asm

* include/ : 内核需要的头文件 : 
> 1. const.h & proc.h & protect.h & sconst.inc & type.h : 数据结构和常量
> 2. global.h : 全局变量定义
> 3. proto.h & string.h : 函数原型申明

* kernel/ : 
> 1. global.c : 全局变量定义
> 2. clock.c & i8259.c & proc.c & protect.c & start.c & syscall.asm : proto.h中申明的函数定义
> 3. kernel.asm : 内核
> 4. main.c : 多进程PCB初始化,跳入执行进程

* lib/ : 
> 1. kliba.asm : 可重用的asm函数
> 2. string.asm : 内存操作函数
> 3. klib.c : ......




## 主要更新进度:
......
### 19年7月14日更新:添加了多进程
> 看restart()函数(kernel.asm)

### 19年7月20日更新:添加了基本I/O

1. 键盘输入->屏幕
2. 多控制台(F4, F5, F6)

> 这几天学车, 暂时不更新master了, 学车会到25号, 然后驾校休息到8月5号.
>
> 所以从现在到25号不会更新master, 但是会更新OSplus和master的readme(具体解释下多进程和基本I/O)...