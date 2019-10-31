# Lab 1: Booting a PC
Lab1的内容是教我们搭建、熟悉环境。包括使用**QEMU**虚拟机，以及使用**GDB**来调试。接着就是带我们追踪整个xv6启动的过程。由于我对汇编、内存地址访问的内容全部忘光了（本来就不会。。。），当中碰到不清楚的，只好碰到一样了解一样了。

## 启动的步骤
* 通电，CPU运行BIOS, BIOS加载并运行boot loader
* boot loader加载kernel，kernel开始运行

### BIOS的运行
BIOS位于内存地址中**0x000F0000**到**0x000FFFFF**这64KB的空间。当机器启动时最先运行这部分的代码，执行最基本的初始化（激活显卡等），并且从持久存储设备（软盘、硬盘、CD-ROM或者网络）加载boot loader。
boot loader会读取每个引导设备的第一个扇区（512字节）的内容。
BIOS会检查这些扇区的最后两个字节是否为**0xAA55**，如果是就代表该扇区是可引导的。
然后BIOS就会将这512个字节加载到内存中**0x7C00**到**0x7DFF**的位置，接着一个`jmp`指令直接跳过去，把控制交给boot loader。

### Boot loader
Boot loader的代码在`boot/boot.S`和`boot/main.c`中，我们需要了解一下这些代码中干了点啥。
* 先看一下`boot.S`:
```
.set PROT_MODE_CSEG, 0x8         # kernel code segment selector
.set PROT_MODE_DSEG, 0x10        # kernel data segment selector
.set CR0_PE_ON,      0x1         # protected mode enable flag
```
`.set`相当于是汇编当中的`#define`吧，我是这样理解的。所以先定义一些后面用的常量。

* 做一些初始化工作:
```
.globl start
start:
  .code16                     # Assemble for 16-bit mode
  cli                         # Disable interrupts
  cld                         # String operations increment

  # Set up the important data segment registers (DS, ES, SS).
  xorw    %ax,%ax             # Segment number zero
  movw    %ax,%ds             # -> Data Segment
  movw    %ax,%es             # -> Extra Segment
  movw    %ax,%ss             # -> Stack Segment
```
`globl`告诉链接器后面跟的是一个全局可见的名字。
`.code16`代表生成16位机器码。
`cli`关闭所有中断，因为在启动的过程中，不能让外面给你中断了，那就毁了。
`cld`，不懂。。好像暂时也没有必要懂，先放着吧。
然后就是将`DS`,`ES`,`SS`三个寄存器置为`0`，这很容易看懂。

* 打开A20地址线：
```
  # Enable A20:
  #   For backwards compatibility with the earliest PCs, physical
  #   address line 20 is tied low, so that addresses higher than
  #   1MB wrap around to zero by default.  This code undoes this.
seta20.1:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.1

  movb    $0xd1,%al               # 0xd1 -> port 0x64
  outb    %al,$0x64

seta20.2:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.2

  movb    $0xdf,%al               # 0xdf -> port 0x60
  outb    %al,$0x60
```
这一部分是比较难理解的。
要有这一部分的目的是为了兼容旧的PC，至于为啥这样就兼容了我还是不太明白。。。
旧的PC地址空间就只有**1MB**（20位），现在的PC刚打开时候依然只支持这么多。
如果寻址寻着寻着超过了，那超过20位的部分依然是0。
比如找一个地址`0x100002`，在没有开启**A20**地址线的情况下就会寻到`0x2`去。
看到网上说这种情况叫**卷绕机制**。
这里打开A20地址线的方法是通过键盘控制器，具体原理我就不明白了。
做法就是先往**0x64**端口写入数据`0xd1`，然后再往**0x60**端口写入数据`0xdf`。
往两个端口写入前，都先从端口读取（通过`inb`），来判断端口是否被占用。
一直到端口空出来时，才往里面写入。

* 从**Real Mode**切换到**Protected Mode**
```
  # Switch from real to protected mode, using a bootstrap GDT
  # and segment translation that makes virtual addresses
  # identical to their physical addresses, so that the
  # effective memory map does not change during the switch.
  lgdt    gdtdesc
  movl    %cr0, %eax
  orl     $CR0_PE_ON, %eax
  movl    %eax, %cr0

  # Jump to next instruction, but in 32-bit code segment.
  # Switches processor into 32-bit mode.
  ljmp    $PROT_MODE_CSEG, $protcseg
```
由于进入保护模式后就不再是按照原来的寻址方式了。
保护模式下，内存会分成"段"。
而每一段的信息会以段描述符的形式维护在一个**GDT**（全局描述符表）中。
此时，**CS**寄存器的值代表着要访问的这“段”内存对应的段描述符在GDT中对应的下标。
因此要进入保护模式的话，一定要准备好**GDT**这张表。
在整个系统中**GDT**只有一张，它可以被放在内存中的任何位置，但CPU必须知道它的位置（即基地址）。
**GDTR**（全局描述符表寄存器）即是专用用来存放这一信息的寄存器，它记录了**GDT**在内存中的基地址和其表长（以字节为单位）。
`lgdt gdtdesc`这条指令将`gdtdesc`处的内容加载到**GDTR**。
`gdtdesc`定义在下面：
```
gdtdesc:
  .word   0x17                            # sizeof(gdt) - 1
  .long   gdt                             # address gdt
```
前面的**16bit**代表GDT的**表长度减1**，这里是`0x17`即`23`，因此实际是24，这里找到一个地方印证这一点：[link](
https://en.wikibooks.org/wiki/X86_Assembly/Global_Descriptor_Table)。
至于为什么要减1并没有解释。
我自己瞎猜的话，由于刚开始未设置的时候`limit`就被设置为`0xFF`，因此`65535`就被占用无法表示了。
而且既然要使用**GDT**了，怎么可能表还是空的呢？那就什么内存都不能访问了吧。

接着的**32bit**就是gdt的地址了，
然后我们可以看到`gdt`的定义：
```
# Bootstrap GDT
.p2align 2                                # force 4 byte alignment
gdt:
  SEG_NULL				# null seg
  SEG(STA_X|STA_R, 0x0, 0xffffffff)	# code seg
  SEG(STA_W, 0x0, 0xffffffff)	        # data seg
```
`.p2align`指令的意思是`2`的指数对齐。
这里操作数也是`2`，因此是`4`字节对齐。

然后我们可以看一下**gdt**表中每一项的结构：
![gdt entry](https://raw.githubusercontent.com/Irving-cl/mit-6.828-learning/master/lab1/gdt_entry.gif)

可以看到每一条占8个字节，因此3条总共24个字节，对应**gdtr**寄存器`limit`的值23。
首先第一段`SEG_NULL`，全零，为什么要有这个东西我并不清楚。。。
接着我觉得不必把code seg和data seg完全展开看了。
`SEG`这个宏就接受3个参数，对应entry中的`type`，`base`，`limit`。
就是说通过`SEG`宏我们在entry里设置了这三样。
剩下的几位的设置包含在`SEG`宏里面了，我就暂时不管了。
可以看到，这两个seg的基地址都是`0x0`。
而段的`limit`这里应该都是`0xffffffff`。
这里要注意，在entry中`limit`总共就占**20bit**，不可能是**32bit**的。
而`SEG`宏当中也是取了第三个参数中的**20bit**，至于为何依然不得而知。
因此最后得到的`limit`为`0xfffff`。
`SEG`中还将`G bit`置为`1`，代表段界限单位为**4k**。
所以总的`limit`为`4k * 2^20 = 4GB`。
最后看一下`type`。
代码段中，`STA_X`代表可执行，`STA_R`代表可读。
数据段中，`STA_W`代表可写。
到这里，**gdt**已经准备完毕，并且**gdt**的信息已加载到**gdtr**寄存器中。

然后就要真正打开保护模式了。
x86有4个控制寄存器：**CR0**，**CR1**，**CR2**，**CR3**。
而保护模式的开关就在**CR0**控制寄存器中。
由于**CR0**无法被直接修改，先将它加载到**ax**中，然后置上`PE`位。
它是**CR0**中从低到高的第一位。
至此就正式进入保护模式了。

接下来是一个跳转命令：`ljmp    $PROT_MODE_CSEG, $protcseg`。
此时的第一个参数为`0x8`。
正如刚才所说，此时这个参数表示**gdt**表中的下标。
然后因为**gdt**表最多有**8192**项，只需前**13**位，剩下3位可作他用。
因此这里段的下标其实就是**1**，也就是刚才设置好的代码段，基地址为`0x0`，limit为`0xfffff`。
然后偏移量是`$protcseg`，直接跳到下一段代码：

```
protcseg:
  # Set up the protected-mode data segment registers
  movw    $PROT_MODE_DSEG, %ax    # Our data segment selector
  movw    %ax, %ds                # -> DS: Data Segment
  movw    %ax, %es                # -> ES: Extra Segment
  movw    %ax, %fs                # -> FS
  movw    %ax, %gs                # -> GS
  movw    %ax, %ss                # -> SS: Stack Segment

  # Set up the stack pointer and call into C.
  movl    $start, %esp
  call bootmain

  # If bootmain returns (it shouldn't), loop.
spin:
  jmp spin
```
这里设置这些寄存器的用意我暂时不明白。
下面就直接调用`bootmain`了，接着就是由C代码实现的了，在`boot/main.c`中。
至此，`boot.S`的工作全部完成了。

## ELF格式
然后稍微讲了一些ELF格式。

### Program Section
程序段中有这几样是我们需要关注的：
*    .text: 存放可执行命令。
*    .rodata: 存放只读数据，比如一些常量的字符串。
*    .data: 存放初始化过的全局变量。
*    .bss: 存放未初始化的全局变量。
因为在C中未初始化的全局变量全是0，所以也不需要给.bss部分存储内容，只需要记录它的位置和大小就行了。

### VMA and LMA
VMA是**Virtual Memory Address**，LMA是**Load Memory Address**。
通过`objdump`可以看到这一信息。
可是LMA是加载到实际内存的地址，指的是物理地址，怎么会在编译完的时候就决定了呢？
这个问题暂且留着吧，等学到后面的时候再回过头来解决。
