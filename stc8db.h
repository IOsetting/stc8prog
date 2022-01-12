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

#ifndef __STC8DB_H__
#define __STC8DB_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#define PROTOCOL_UNSUPP    0x0000
#define PROTOCOL_STC8GH    0x0001
#define PROTOCOL_STC8AF    0x0002
#define PROTOCOL_STC15B    0x0003
#define PROTOCOL_STC15     0x0004


typedef struct _model_paras {
    char name[24];
    uint16_t magic_code;
    uint16_t protocol;
    uint32_t total_flash;
    uint32_t code_size;
    uint32_t eeprom_size;
} stc_model_t;

typedef struct _protocol_paras {
    char name[12];
    uint16_t id;
    uint8_t info_pos_fosc;
    uint8_t baud_switch[9];
    uint8_t baud_check[6];
    uint8_t flash_erase[6];
    uint8_t flash_write[7];
} stc_protocol_t;

const stc_model_t* model_lookup(uint16_t code);
const stc_protocol_t* protocol_lookup(uint16_t id);

#endif