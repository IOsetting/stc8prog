#  stc8prog - STC MCU ISP flash tool

stc8prog is a command line flash programming tool for STC 8051 series 
microcontrollers, Currently, only STC8H/STC8G/STC8A, STC15
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
  -h, --help                    display this message
  -p, --port <device>           set device path
  -s, --speed <baud>            set download baudrate
  -r, --reset <msec>            make reset sequence by pulling low dtr
  -r, --reset <cmd> [args] ;    command to perform reset or power cycle
  -f, --flash <file>            flash chip with data from hex file
  -e, --erase                   erase the entire chip
  -d, --debug                   enable debug output
  -v, --version                 display version information

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

By default PlatformIO places packages at `/home/[username]/.platformio/packages`
1. create a folder named `tool-stc8prog` under it
1. and place stc8prog executable under it.

### 2. Configurate platformio.ini

Create a new env in platformio.ini, it can be a clone of existing one, change the name and change `upload_protocol` 
option to `custom`, then configurate the rest accordingly, for example:
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
If you want to reset the MCU by pulling DTR line low, add `-r 5` to upload_flags config  
If you want to make this env default, set it to `default_envs`
```
[platformio]
default_envs = stc8h3k32s2-stc8prog
```

For more options, please read [platformio section_env_upload](https://docs.platformio.org/en/latest/projectconf/section_env_upload.html)

### 3. First time upload

You can trigger upload with hotkey `Ctrl`+`Alt`+`U`. 

If there are any erros and you want to see more detailed logs, run it in the verbose way:

1. Click PlatformIO icon in the left navigation panel
1. Expand `[your env name]` in `PROJECT TASKS`
1. Expand `Advanced`
1. Click `Verbose Upload`, this will output the full log of all commands


# Build From Source

#### Install DEV-tools
* Linux
```shell
sudo apt install build-essential
```
* macOS
```shell
xcode-select --install
```

#### Checkout the project  
```shell
git clone https://github.com/IOsetting/stc8prog.git
```
#### Compile the project
```shell
cd stc8prog
make
```
#### Optional: install to system path
```shell
sudo make install
```

# Gentoo linux

Package dev-embedded/stc8prog-9999 located in unoficial rasdark overlay.
Resulting binary will be from last commit from https://github.com/IOsetting/stc8prog.git

# Build From Source in Windows under Cygwin

1. Run Cygwin setup
2. Install 'gcc-core' and 'make' packages
3. Use command-line git to pull latest stc8prog source to desired folder
4. Run Cygwin terminal
5. Navigate to folder with stc8prog source (windows disks are mapped to /cygdrive; eg. to navigate to 'stc8prog' folder located on 'D' drive, use 'cd /cygdrive/d/stc8prog/')
6. Run make to build binary

* By default, Cygwin's gcc binary requires cygwin1.dll to work correctly, just copy it from Cygwin installation dir bin folder