#  stc8prog - STC MCU ISP flash tool

stc8prog is a command line flash programming tool for STC 8051 series 
microcontrollers, Currently, only STC8H/STC8G/STC8A8K64D4, STC15
(FWVER > 0x72) series are supported.

STC microcontrollers have an UART/USB based boot strap loader (BSL). It 
utilizes a packet-based protocol to flash the code memory and IAP memory over 
a serial link. This is referred to as in-system programming (ISP). This tool is
built according to the ISP described in STC8H datasheet.

### Tested MCU Types:

* STC8G1K08A
* STC8H1K08
* STC8H3K32S2
* STC8A8K64D4

# How To Use

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

