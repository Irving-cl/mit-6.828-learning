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

### Questions
* At what point does the processor start executing 32-bit code? What exactly causes the switch from 16- to 32-bit mode?
这个非常显而易见了，开启32位模式后gdb打出来的东西都不一样了，中间还插了一行字。。。所以就是跳转到`0x7c32`之后开始的。
实际转换做的操作就是打开`cr0`寄存器上的开关，就是从`0x7c23`到`0x7c2a`的这三条指令。
* What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?
* Where is the first instruction of the kernel?
* How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?
