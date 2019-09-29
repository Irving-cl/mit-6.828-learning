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
