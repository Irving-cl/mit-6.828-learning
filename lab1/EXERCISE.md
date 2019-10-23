# Exercise

## Exercise 3
跟着GDB把`boot.S`的部分走了一遍，是用`si`命令一步一步走的：

```
[   0:7c00] => 0x7c00:	cli
[   0:7c01] => 0x7c01:	cld
[   0:7c02] => 0x7c02:	xor    %ax,%ax
[   0:7c04] => 0x7c04:	mov    %ax,%ds
[   0:7c06] => 0x7c06:	mov    %ax,%es
[   0:7c08] => 0x7c08:	mov    %ax,%ss
[   0:7c0a] => 0x7c0a:	in     $0x64,%al
[   0:7c0c] => 0x7c0c:	test   $0x2,%al
[   0:7c0e] => 0x7c0e:	jne    0x7c0a
[   0:7c10] => 0x7c10:	mov    $0xd1,%al
[   0:7c12] => 0x7c12:	out    %al,$0x64
[   0:7c14] => 0x7c14:	in     $0x64,%al
[   0:7c16] => 0x7c16:	test   $0x2,%al
[   0:7c18] => 0x7c18:	jne    0x7c14
[   0:7c1a] => 0x7c1a:	mov    $0xdf,%al
[   0:7c1c] => 0x7c1c:	out    %al,$0x60
[   0:7c1e] => 0x7c1e:	lgdtw  0x7c64
[   0:7c23] => 0x7c23:	mov    %cr0,%eax
[   0:7c26] => 0x7c26:	or     $0x1,%eax
[   0:7c2a] => 0x7c2a:	mov    %eax,%cr0
[   0:7c2d] => 0x7c2d:	ljmp   $0x8,$0x7c32
The target architecture is assumed to be i386
=> 0x7c32:	mov    $0x10,%ax
=> 0x7c36:	mov    %eax,%ds
=> 0x7c38:	mov    %eax,%es
=> 0x7c3a:	mov    %eax,%fs
=> 0x7c3c:	mov    %eax,%gs
=> 0x7c3e:	mov    %eax,%ss
=> 0x7c40:	mov    $0x7c00,%esp
=> 0x7c45:	call   0x7d19
```
会发现这些指令和`boot.S`没啥区别，就是把相当于把`boot.S`里的那些诸如`start`,`seta20.1`这样的符号换成了具体的地址。

### 追踪readsect的过程
先看一下`readsect`的代码：
```
void
readsect(void *dst, uint32_t offset)
{
	// wait for disk to be ready
    waitdisk();

    outb(0x1F2, 1);		// count = 1
    outb(0x1F3, offset);
    outb(0x1F4, offset >> 8);
    outb(0x1F5, offset >> 16);
    outb(0x1F6, (offset >> 24) | 0xE0);
    outb(0x1F7, 0x20);	// cmd 0x20 - read sectors

    // wait for disk to be ready
    waitdisk();

    // read a sector
    insl(0x1F0, dst, SECTSIZE/4);
}
```
在`boot.asm`中看到`readsect`的位置在`0x7c7c`，所以在这里下个断点。开始后先是基本的调用过程和传参：
```
=> 0x7c7c:	push   %ebp
=> 0x7c7d:	mov    %esp,%ebp
=> 0x7c7f:	push   %edi
=> 0x7c80:	mov    0xc(%ebp),%ecx
```
进入`readsect`后，调用第一个函数`waitdisk`：
```
	// wait for disk reaady
	while ((inb(0x1F7) & 0xC0) != 0x40)
		/* do nothing */;
```
对应调用它的汇编:
```
=> 0x7c83:	call   0x7c6a
=> 0x7c6a:	push   %ebp
=> 0x7c6b:	mov    $0x1f7,%edx
=> 0x7c70:	mov    %esp,%ebp
=> 0x7c72:	in     (%dx),%al
=> 0x7c73:	and    $0xffffffc0,%eax
=> 0x7c76:	cmp    $0x40,%al
=> 0x7c78:	jne    0x7c72
=> 0x7c7a:	pop    %ebp
=> 0x7c7b:	ret
```
这一部分比较容易理解，就是不断从端口读入，然后位操作，再作比较，条件跳转，直到退出循环。

接着是一串`out`的调用，第一条`outb(0x1F2, 1)`对应的汇编：
```
=> 0x7c88:	mov    $0x1f2,%edx
=> 0x7c8d:	mov    $0x1,%al
=> 0x7c8f:	out    %al,(%dx)
```
这一串`out`都是类似的，就不赘述了。这一串`out`应该是叫磁盘去读位于`offset`处的**1**个sect。

然后又是一个`waitdist`调用，等磁盘完成工作。

现在，可以把磁盘出来的内容加载到内存了，对应`insl`那行代码：
```
=> 0x7cd6:	cld
=> 0x7cd7:	repnz insl (%dx),%es:(%edi)
=> 0x7cd7:	repnz insl (%dx),%es:(%edi)
=> 0x7cd7:	repnz insl (%dx),%es:(%edi)
=> 0x7cd7:	repnz insl (%dx),%es:(%edi)
...
```
`cld`指令清除方向标志，是为了后续rep连用的操作，具体我不太懂。
接下来是大量重复的从端口读入到内存，每次读**4**个byte。
每次`%edi`寄存器的值也会加**4**，从一开始的`0x10000`增长到`0x10200`。正好是读了512个byte。

读取sect的for循环的开始和结束：
```
    for (; ph < eph; ph++)
```
显然就是比较开始和比较结束了，对应的汇编是：
```
=> 0x7d51:	cmp    %esi,%ebx
=> 0x7d53:	jae    0x7d6b
```
比较满足条件后，就会跳出循环继续执行。

跳出循环之后就准备运行内核了，只有一条指令：
```
=> 0x7d6b:	call   *0x10018
```
这里就跳转到内核的代码了。

### Questions
* At what point does the processor start executing 32-bit code? What exactly causes the switch from 16- to 32-bit mode?

这个非常显而易见了，开启32位模式后gdb打出来的东西都不一样了，中间还插了一行字。。。所以就是跳转到`0x7c32`之后开始的。
实际转换做的操作就是打开`cr0`寄存器上的开关，就是从`0x7c23`到`0x7c2a`的这三条指令。

* What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?

boot loader的最后一条指令： `call *0x10018`，kernel的第一条指令： `movw $0x1234,0x472`。

* Where is the first instruction of the kernel?

地址是在`0x10000c`，代码在`entry.S`中。

* How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?

通过Program Header有多少条entry，就需要加载多少段。这个信息在一开始加载的ELF Header中。


## Exercise 4
这一部分让我们学习C指针。本以为这个总归是会的，还是发现了一些新姿势。
在它给我们的`pointers.c`里：
```
    int *c;

    // ...
    3[c] = 302;
```
第一次知道指针还能这样写，石锤了我以前没好好学习。
从结果看这里这行代码就等价于：
```
    c[3] = 302;
```


## Exercise 5
这个练习有毒。。当然也是我自己的关系。
一开始把`-Ttext 0x7c00`改成`-Ttext 0x8c00`，然后断点设在`0x8c00`，结果直接挂了。。。
所以这里即时改这个`-Ttext`，这段bootloader依旧是被加载到`0x7c00`这里的。
这才回想起来开头的地方有说，这是BIOS做的，是固定的。
只是编译时，其他的一些算出来的地址会从基于`0x7c00`，变为基于`0x8c00`。
因此跳转的时候就会出错了。
发现运行到这一句的时候：
```
[   0:7c2d] => 0x7c2d:	ljmp   $0x8,$0x8c32
```
跳转就会失败了，是跳都跳不过去了，这我不知道为什么，但是`0x8c32`那里也不是正确的代码。


## Exercise 6
一开始的时候`0x100000`那里全是空的。
进入kernel之后那里就变成了kernel的代码。
经过实际验证也确实如此，断点打在`0x10000c`：
```
(gdb) x/8x 0x100000
0x100000:	0x1badb002	0x00000000	0xe4524ffe	0x7205c766
0x100010:	0x34000004	0x0000b812	0x220f0011	0xc0200fd8
```
看`kernel.asm`发现一模一样：
```
.globl entry
entry:
        movw    $0x1234,0x472                   # warm boot
f0100000:       02 b0 ad 1b 00 00       add    0x1bad(%eax),%dh
f0100006:       00 00                   add    %al,(%eax)
f0100008:       fe 4f 52                decb   0x52(%edi)
f010000b:       e4                      .byte 0xe4
```


## Exercise 7
比较容易理解。
在执行`movl %eax, %cr0`之前，`0x100000`那里是内核代码，`0xf0100000`那里全是0。
执行之后，这两处都是内核代码，因为`0xf0100000`也被映射到了`0x100000`这里。
```
=> 0x100025:	mov    %eax,%cr0

Breakpoint 1, 0x00100025 in ?? ()
(gdb) x/8x 0xf0100000
0xf0100000 <_start+4026531828>:	0x00000000	0x00000000	0x00000000	0x00000000
0xf0100010 <entry+4>:	0x00000000	0x00000000	0x00000000	0x00000000
(gdb) x/8x 0x100000
0x100000:	0x1badb002	0x00000000	0xe4524ffe	0x7205c766
0x100010:	0x34000004	0x0000b812	0x220f0011	0xc0200fd8
(gdb) si
=> 0x100028:	mov    $0xf010002f,%eax
0x00100028 in ?? ()
(gdb) x/8x 0xf0100000
0xf0100000 <_start+4026531828>:	0x1badb002	0x00000000	0xe4524ffe	0x7205c766
0xf0100010 <entry+4>:	0x34000004	0x0000b812	0x220f0011	0xc0200fd8
(gdb) x/8x 0x100000
0x100000:	0x1badb002	0x00000000	0xe4524ffe	0x7205c766
0x100010:	0x34000004	0x0000b812	0x220f0011	0xc0200fd8
```
如果把`mov    %eax,%cr0`注释掉，在`=> 0x10002a:	jmp    *%eax`之后的指令就会出错了。
因为要跳转去的地址不会被映射到正确的地址。
qemu也显示了错误信息：
```
qemu: fatal: Trying to execute code outside RAM or ROM at 0xf010002c
```


## Exercise 8
直接模仿10进制和16进制的做法。
先从参数列表中获取要打印的值。
然后设置全局变量`base`为8。
然后跳转到打印数字那里。
```
        // (unsigned) octal
        case 'o':
            num = getuint(&ap, lflag);
            base = 8;
            goto number;
```
修改完之后再启动qemu，里面那句打印会变成正确的值：
```
6828 decimal is 15254 octal!
```

### Questions
1.    Explain the interface between printf.c and console.c. Specifically, what function does console.c export? How is this function used by printf.c?

`printf.c`中的`cputchar`是在`console.c`中实现的。这个函数也作为参数去调用`vprintfmt`。

2.    Explain the following from console.c:
```
      if (crt_pos >= CRT_SIZE) {
          int i;
          memmove(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
          for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
              crt_buf[i] = 0x0700 | ' ';
              crt_pos -= CRT_COLS;
      }
```
这里应该是输出屏幕满了，将第一行去掉，第二行开始每一行都往前挪，把最后一行空出来。

3.    Trace the execution of the following code step-by-step:
```
int x = 1, y = 3, z = 4;
cprintf("x %d, y %x, z %d\n", x, y, z);
```
*    In the call to cprintf(), to what does fmt point? To what does ap point?

`fmt`显然指向格式字符串，地址为`0xf01018d7`:
>(gdb) si
>=> 0xf01000a0 <i386_init+12>:	push   $0xf01018d7
>0xf01000a0	26	        cprintf("x %d, y %x, z %d\n", x, y, z);
>(gdb) x/1s 0xf01018d7
>0xf01018d7:	"x %d, y %x, z %d\n"

`ap`指向可变参数列表中的第一项，在这里就是`x`,也是内存地址最低的参数。
>(gdb) si
>=> 0xf0100911 <cprintf+6>:	lea    0xc(%ebp),%eax
>31		va_start(ap, fmt);
从`%ebp`开始向上，存储的分别是上次的`%ebp`，返回地址，`fmt`参数。接着就是可变参数列表的第一项了，所以是`0xc(%ebp)`。

*    List (in order of execution) each call to cons_putc, va_arg, and vcprintf. For cons_putc, list its argument as well. For va_arg, list what ap points to before and after the call. For vcprintf list the values of its two arguments.

`cons_putc`就一个参数，传递时放在`%eax`中。

`va_arg`这个不太明白它的意思。
运行到那里的时候`ap`指向栈中存放可变参数的地址，存在`%eax`中。运行`va_arg`之后，`%eax`就变成了那个地址里的值，也就是可变参数的值。

