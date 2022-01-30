#  stc8prog - STC MCU ISP flash tool

stc8prog is a command line flash programming tool for STC 8051 series 
microcontrollers, Currently, only STC8H/STC8G/STC8A8K64D4, STC15
(FWVER > 0x72) series are supported.

STC microcontrollers have an UART/USB based boot strap loader (BSL). It 
utilizes a packet-based protocol to flash the code memory and IAP memory over 
a serial link. This is referred to as in-system programming (ISP). This tool is
built according to the ISP described in STC8H datasheet.

### Tested MCU Types:

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

# How To Use

## CLI Tool

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

### Check
Communicate with MCU without changing anything. It will communicate in 2400 baud first, then switch to a higher baud for a second check
```bash
# 2400 baud -> 115200 baud
./stc8prog -p /dev/ttyUSB0
```
You can specify other baud with parameter `-s` 
```bash
# 2400 baud -> 1152000 baud
./stc8prog -p /dev/ttyUSB0 -s 1152000
```

### Erase Flash
This will erase the flash.
```bash
./stc8prog -p /dev/ttyUSB0 -e
```

### Write Flash
Suggest to erase before writing flash
```bash
./stc8prog -p /dev/ttyUSB0 -e -f foo.hex
```
High baudrate will speed up the writing
```bash
./stc8prog -p /dev/ttyUSB0 -s 1152000 -e -f foo.hex
```

## PlatformIO Integration

### 1. Add it to packages 

By default PlatformIO places packages at `/home/[username]/.platformio/packages`, create a folder named
`tool-stc8prog` under it, and place stc8prog executable under it.

### 2. Configurate platformio.ini

Create a new env in platformio.ini, it can be a clone of existing one, change the name and change `upload_protocol` 
option to custom, then configurate the rest accordingly, for example:
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
If you want to make this env default, set it to `default_envs`
```
[platformio]
default_envs = stc8h3k32s2-stc8prog
```

For more options, please read [platformio section_env_upload](https://docs.platformio.org/en/latest/projectconf/section_env_upload.html)

### 3. First time upload

You can trigger upload with hotkey `Ctrl`+`Alt`+`U`. 

If there are any erros and you want to see more detailed logs, run it in the verbose way:

* Click PlatformIO icon in the left navigation panel
* Expand `[your env name]` in `PROJECT TASKS`
* Expand `Advanced`
* Click `Verbose Upload`, this will output the full log of all commands


# Build From Source

Install DEV-tools  
```shell
sudo apt install build-essential
```
Checkout the project  
```shell
git clone https://github.com/IOsetting/stc8prog.git
```
Compile the project
```shell
cd stc8prog
make
```

