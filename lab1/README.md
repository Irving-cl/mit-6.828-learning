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

