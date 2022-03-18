#  stc8prog - STC ISP 烧录工具

stc8prog 是一个用于STC 8051系列微控制器的命令行烧录工具, 当前只支持 STC8H/STC8G/STC8A8K64D4, STC15
(FWVER > 0x72) 系列型号. 

因为stcgal不支持STC8H/STC8G/STC8A8K64D4且长时间未更新, 这个工具是为了解决在Linux下烧录
STC8H/STC8G/STC8A8K64D4 系列芯片的问题.


### 已测试通过的型号:

* STC8A8K64S4A12
* STC8A8K64D4
* STC8G1K08A
* STC8H1K08
* STC8H3K32S2
* STC8H3K64S4
* STC8H8K64U
* STC15W104
* STC15F104W
* STC15W408AS
* STC15F2K60S2

# 使用

## 命令行工具

烧录时需要对MCU重新加电, 过程和stcgal, 以及官方的stc-isp工具是一样的.

```
Usage: stc8prog [options]...
  -h, --help            display this message
  -p, --port <device>   set device path
  -s, --speed <baud>    set download baudrate
  -r, --dtr reset<msec> make reset sequence by pulling low dtr
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

## 集成到PlatformIO

### 1. 添加到 packages 

PlatformIO的packages目录默认路径是 `/home/[username]/.platformio/packages`
1. 在下面新建一个目录 tool-stc8prog
1. 将可执行文件 stc8prog 复制到这个目录下

### 2. Configurate platformio.ini

在项目的platformio.ini中新建一个env, 可以用现有的env复制创建, 修改upload为custom, 并增加对应的配置参数, 下面是一个例子
```
[env:stc8h3k32s2-stc8prog]
platform = intel_mcs51
board = stc8h3k32s2
upload_protocol = custom
upload_port = /dev/ttyUSB0
upload_flags =
    -p
    $UPLOAD_PORT
    -s
    1152000
    -e
upload_command = ${platformio.packages_dir}/tool-stc8prog/stc8prog $UPLOAD_FLAGS -f $SOURCE
```
如果需要将其设置为默认, 在 default_envs 中设置
```
[platformio]
default_envs = stc8h3k32s2-stc8prog
```
更多说明和配置项请参考文档 [platformio section_env_upload](https://docs.platformio.org/en/latest/projectconf/section_env_upload.html)

### 3. 烧录

通过菜单或快捷键 `Ctrl`+`Alt`+`U` 开始烧录, 如果过程中出现错误, 需要看到更多的日志信息, 可以通过verbose方式

1. 点击左侧导航栏中PlatformIO的图标
1. 在`PROJECT TASKS`中, 点击展开`[your env name]`
1. 在里面点击展开 `Advanced`
1. 点击 `Verbose Upload`, 这样会输出完整的命令行信息


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

