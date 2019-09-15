# Lab 1: Booting a PC
Lab1的内容是教我们搭建、熟悉环境。包括使用**QEMU**虚拟机，以及**GDB**来使用调试。接着就是带我们追踪整个xv6启动的过程。由于我对汇编、内存地址访问的内容全部忘光了（本来就不会。。。），当中碰到不清楚的，只好碰到一样了解一样了。

## 启动的步骤
* 通电CPU运行BIOS
* BIOS加载并运行Bootloader
* Bootloader接在kernel，kernel开始运行


