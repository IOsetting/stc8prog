#  stc8prog - STC ISP 烧录工具

stc8prog 是一个用于STC 8051系列微控制器的命令行烧录工具, 当前只支持 STC8H/STC8G/STC8A8K64D4, STC15
(FWVER > 0x72) 系列型号. 

因为stcgal不支持STC8H/STC8G/STC8A8K64D4且长时间未更新, 这个工具是为了解决在Linux下烧录
STC8H/STC8G/STC8A8K64D4 系列芯片的问题.


### 已测试通过的型号:

* STC8G1K08A
* STC8H1K08
* STC8H3K32S2
* STC8A8K64D4

# 使用

烧录时需要对MCU重新加电, 过程和stcgal, 以及官方的stc-isp工具是一样的.

```
Usage: stc8prog [options]...
  -h, --help            display this message
  -p, --port <device>   set device path
  -s, --speed <baud>    set download baudrate
  -f, --flash <file>    flash chip with data from hex file
  -e, --erase           erase the entire chip
  -d, --debug           enable debug output
  -v, --version         display version information

Baudrate options: 
   4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 500000, 576000,
   921600, 1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000,
   4000000
```

### 检查

与MCU作握手通信, 过程不作任何写入. 过程中先通过2400波特率握手, 再切换到115200波特率握手
```bash
# 2400 baud -> 115200 baud
./stc8prog -p /dev/ttyUSB0
```
可以使用参数`-s`指定其它的波特率(必须在可选的波特率列表里)
```bash
# 2400 baud -> 1152000 baud
./stc8prog -p /dev/ttyUSB0 -s 1152000
```

### 擦除
擦除整块MCU
```bash
./stc8prog -p /dev/ttyUSB0 -e
```

### 烧录
建议在烧录时带上擦除参数
```bash
./stc8prog -p /dev/ttyUSB0 -e -f foo.hex
```
使用更高的波特率可以加速烧录
```bash
./stc8prog -p /dev/ttyUSB0 -s 1152000 -e -f foo.hex
```

# 从源码构建

安装 DEV-tools  
```shell
sudo apt install build-essential
```
检出项目
```shell
# GitHub
git clone https://github.com/IOsetting/stc8prog.git
# Gitee
git clone https://gitee.com/iosetting/stc8prog.git
```
编译
```shell
cd stc8prog
make
```

