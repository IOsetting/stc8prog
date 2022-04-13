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
#include "stc8db.h"
#include "userial.h"

/* OS-abstract serial instance */
extern userial_t serial;

typedef unsigned char BYTE;
typedef unsigned short WORD;

#define LOBYTE(w) ((BYTE)(WORD)(w))
#define HIBYTE(w) ((BYTE)((WORD)(w) >> 8))

#define FUSER 24000000L             // STC8H MCU default frequency
#define RL(n) (65536 - FUSER/4/(n)) // STC8H UART baudrate calculator

#define MINBAUD 2400
#define MAXBAUD 115200

#define DEBUG_PRINTF(...) if(debug){printf(__VA_ARGS__);}

/* stc8prog.c */

/***
 * @brief detect chip
 * @param recv          - [out] chip detect data destination
 * @param retry_count   - [in] handshake retry count
 * 
 * @return              - 0 if chip detected,
 *                        error code otherwise
 */ 
extern int32_t chip_detect(uint8_t * restrict const recv,
                           const uint16_t retry_count);

extern void set_debug(uint8_t val);
extern int baudrate_set(const stc_protocol_t * stc_protocol, unsigned int speed, uint8_t *recv);
extern int baudrate_check(const stc_protocol_t * stc_protocol, uint8_t *recv, uint8_t chip_version);
extern int flash_erase(const stc_protocol_t * stc_protocol, uint8_t *recv);
extern int flash_write(const stc_protocol_t * stc_protocol, unsigned int len);

extern int chip_write(uint8_t *buff, uint8_t len);
extern int chip_read(uint8_t *recv);
extern int chip_read_verify(uint8_t *buf, uint8_t size, uint8_t *recv);

extern int load_hex_file(char *filename);
extern int parse_hex_line(char *theline, int bytes[], int *addr, int *num, int *code);


#endif  /* __STC8PROG_H__ */
