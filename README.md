---
This is my own toy-operating-system when reading the book named 《Oranges' 一个操作系统的实现》
---


# Do-Opeating-System-Youself

## Developer environment:

* Ubuntu 18.04.2 LTS

* NASM version 2.13.02

* gcc 7.4.0

* Bochs x86 Emulator 2.6.9 (source version with debugger)

  **WARNING** : *Bochs must to be source version where you can debug your own OS.*


## Main updates :
**. . . . . .**
### July 14th, 2019 :
* Add multiple processes moudle.

### July 16th, 2019 : 
* Improve source's structure totally, which make it clearer to read. Now, you can use make like this : 

```makefile
make #show usage

make image #make *.bin and write to a.img. after that, you can use bochs to debug at once (^_^)

make clean #clean
```
### July 22th, 2019
* Add standard I/O and support 3 console.

### July 28th, 2019

* Adopt micro-kernel design, add IPC (inner process communication). Now, system-call will adopt IPC to communicate with  SYS_TASK to get some useful info wanted.

