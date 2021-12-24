// Copyright 2021 IOsetting <iosetting@outlook.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __STC8PROG_H__
#define __STC8PROG_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;

#define LOBYTE(w) ((BYTE)(WORD)(w))
#define HIBYTE(w) ((BYTE)((WORD)(w) >> 8))

#define FUSER 24000000L             // STC8H MCU default frequency
#define RL(n) (65536 - FUSER/4/(n)) // STC8H UART baudrate calculator

#define MINBAUD 2400
#define MAXBAUD 115200

#define DEBUG_PRINTF(...) if(debug){printf(__VA_ARGS__);}

/* termios.c */

extern int termios_open(char *path);
extern int termios_set_speed(unsigned int speed);
extern int termios_flush(void);
extern int termios_setup(unsigned int speed, int databits, int stopbits, char parity);
extern int termios_rts(bool enable);
extern int termios_read(void *data, unsigned long len);
extern int termios_write(const void *data, unsigned long len);

/* stc8prog.c */

extern void set_debug(uint8_t val);
extern int entry_detect(uint8_t *buff);
extern int baudrate_set(unsigned int speed, uint8_t *buff);
extern int baudrate_check(uint8_t *buff);
extern int erase_flash(uint8_t *buff);
extern int write_flash(unsigned int len);

extern int chip_write(uint8_t *buff, uint8_t len);
extern int chip_read(uint8_t *buff, uint8_t len);
extern int chip_read_verify(uint8_t *rx, uint8_t len, uint8_t *content);

extern int load_hex_file(char *filename);
extern int parse_hex_line(char *theline, int bytes[], int *addr, int *num, int *code);


#endif  /* __STC8PROG_H__ */
