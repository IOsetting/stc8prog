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

#include "stc8prog.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 255

/* disable printing dots due detect sequence,
 * can be useful on slow terminals
 */
#define SILENT_DETECT

const uint8_t tx_detect[] = {0x7f};
const uint8_t tx_prefix[] = {0x46, 0xb9, 0x6a, 0x00};
const uint8_t tx_suffix[] = {0x16};
const uint8_t rx_prefix[] = {0x46, 0xb9, 0x68, 0x00};
uint8_t debug = 0, memory[65536];

void set_debug(uint8_t val)
{
    debug = val;
}

/***
 * @brief detect chip
 * @param recv          - [out] chip detect data destination
 * @param retry_count   - [in] handshake retry count
 * 
 * @return              - 0 if chip detected,
 *                        error code otherwise
 */ 
int32_t chip_detect(uint8_t * restrict const recv,
                    const uint16_t retry_count)
{
    uint16_t count;
    int ret;

    for (count = 0; retry_count > count; ++count) 
    {
        serial.write(&serial, tx_detect, sizeof(tx_detect));
        if ((ret = chip_read(recv)) <= 0) {
#ifndef SILENT_DETECT
            if (count & 0x1FF == 0) printf("\n");
            if (count & 0x0F == 0) printf(".");
#endif
            continue;
        } else if (*recv == 0x50) {
            return 0;
        } else {
#ifndef SILENT_DETECT
            printf("entry_detect read unmatched ");
#endif
            return -2;
        }
    }
    printf("timeout ");
    return -1;
}

int baudrate_set(const stc_protocol_t * stc_protocol, unsigned int speed, uint8_t *recv)
{
    unsigned int count, ret;
    uint8_t arg_size = sizeof(stc_protocol->baud_switch) - 1;
    uint8_t arg[BUF_SIZE] = {};
    memcpy(arg, stc_protocol->baud_switch, arg_size);
    arg[1] = *(recv + 4);
    if (stc_protocol->id == PROTOCOL_STC15)
    {
        count = 65536 - FUSER / speed;
        arg[3] = (count >> 8) & 0xFF;
        arg[4] = count & 0xFF;
        count = 65536 - FUSER / speed / 2 * 3;
        arg[5] = (count >> 8) & 0xFF;
        arg[6] = count & 0xFF;
    }
    else
    {
        count = 65536 - FUSER / 4 / speed;
        arg[3] = (count >> 8) & 0xFF;
        arg[4] = count & 0xFF;
    }

    chip_write(arg, arg_size);

    for (count = 0; count < 0xFF; ++count)
    {
        if ((ret = chip_read(recv)) <= 0)
        {
            continue;
        }
        else if (*recv == stc_protocol->baud_switch[arg_size])
        {
            return 0;
        }
        else
        {
            printf("baudrate_set read unmatched\n");
            return -1;
        }
    }
    return 1;
}

int flash_write(const stc_protocol_t * stc_protocol, unsigned int len)
{
    uint8_t *recv = (uint8_t [BUF_SIZE]){}, arg[BUF_SIZE] = {};
    uint8_t cnt, count, arg_size = sizeof(stc_protocol->flash_write) - 2;
    unsigned int addr, offset;
    int ret;
    memcpy(arg, stc_protocol->flash_write, arg_size);
    addr = 0;
    offset = 5;

    printf("%6.2f%%", 0.0);
    while (addr < len)
    {
        arg[1] = HIBYTE(addr);
        arg[2] = LOBYTE(addr);

        cnt = 0;
        while (addr < len)
        {
            arg[cnt + offset] = *(memory + addr);
            addr++;
            cnt++;
            if (cnt >= 128)
                break;
        }
        chip_write(arg, cnt + offset);

        for (count = 0; count < 10; ++count)
        {
            if ((ret = chip_read(recv)) <= 0)
            {
                DEBUG_PRINTF("read nothing\n");
                continue;
            }
            else if (*recv == stc_protocol->flash_write[arg_size] 
                && *(recv + 1) == stc_protocol->flash_write[arg_size + 1])
            {
                printf("\b\b\b\b\b\b\b%6.2f%%", addr * 100.0 / len);
                arg[0] = 0x02;
                break;
            }
            else
            {
                printf("flash_write read unmatched\n");
                return -1;
            }
        }
        fflush(stdout);
    }
    printf(" ");
    return 0;
}

int flash_erase(const stc_protocol_t * stc_protocol, uint8_t *recv)
{
    int ret;
    uint8_t count, arg_size = sizeof(stc_protocol->flash_erase) - 1;
    uint8_t arg[BUF_SIZE] = {};
    memcpy(arg, stc_protocol->flash_erase, arg_size);
    chip_write(arg, arg_size);
    for (count = 0; count < 0xFF; ++count)
    {
        if ((ret = chip_read(recv)) <= 0)
        {
            continue;
        }
        else if (*recv == stc_protocol->flash_erase[arg_size])
        {
            return 0;
        }
        else
        {
            printf("erase_flash read unmatched\n");
            return -1;
        }
    }
    return 1;
}

int baudrate_check(const stc_protocol_t * stc_protocol, uint8_t *recv, uint8_t chip_version)
{
    usleep(10000);
    int ret;
    uint8_t count, arg_size = sizeof(stc_protocol->baud_check) - 1;
    uint8_t arg[BUF_SIZE] = {};
    memcpy(arg, stc_protocol->baud_check, arg_size);

    if (chip_version < 0x72)
    {
        chip_write(arg, 1);
    }
    else
    {
        chip_write(arg, arg_size);
    }

    for (count = 0; count < 0xFF; ++count) 
    {
        if ((ret = chip_read(recv)) <= 0)
        {
            continue;
        }
        else if (*recv == stc_protocol->baud_check[arg_size])
        {
            return 0;
        }
        else
        {
            printf("baudrate_check read unmatched\n");
            return -1;
        }
    }
    return 1;
}

int chip_write(uint8_t *buff, uint8_t len)
{
    uint16_t sum;
    uint8_t i, *tx_buf = (uint8_t [BUF_SIZE]){}, *tx_pt = tx_buf;
    memcpy(tx_pt, tx_prefix, sizeof(tx_prefix));
    tx_pt += sizeof(tx_prefix);
    *tx_pt++ = len + 6;
    sum = len + 6 + 0x6a;
    for (i = 0; i < len; i++)
    {
        sum += *(buff + i);
        *tx_pt++ = *(buff + i);
    }
    *tx_pt++ = HIBYTE(sum);
    *tx_pt++ = LOBYTE(sum);
    memcpy(tx_pt, tx_suffix, sizeof(tx_suffix));
    tx_pt += sizeof(tx_suffix);
    serial.write(&serial, tx_buf, tx_pt - tx_buf);
    DEBUG_PRINTF("TX: ");
    for (i = 0; i < tx_pt - tx_buf; i++)
    {
        DEBUG_PRINTF("%02X ", *(tx_buf + i));
    }
    DEBUG_PRINTF("\n");
    return 0;
}

/**
 * return flag;
*/
uint8_t flag_check(uint8_t ch)
{
    static uint8_t rx_flag = 0;
    static uint16_t rx_sum, rx_index, rx_count;
    switch (rx_flag)
    {
        case 8:
            if (ch != 0x16)
            {
                DEBUG_PRINTF("end byte unmatched ");
                rx_flag = 0;
            }
            else
            {
                rx_flag = 9;
                DEBUG_PRINTF("end byte reached ");
            }
            break;

        case 7:
            DEBUG_PRINTF("sum check: 0x%02X ", LOBYTE(rx_sum));
            if (ch != LOBYTE(rx_sum))
            {
                DEBUG_PRINTF("low byte of sum unmatched ");
                rx_flag = 0;
            }
            else
            {
                rx_flag = 8;
            }
            break;

        case 6:
            DEBUG_PRINTF("sum: 0x%02X ", HIBYTE(rx_sum));
            if (ch != HIBYTE(rx_sum))
            {
                DEBUG_PRINTF("high byte of sum unmatched ");
                rx_flag = 0;
            }
            else
            {
                rx_flag = 7;
            }
            break;

        case 5:
            rx_sum += ch;
            rx_index++;
            DEBUG_PRINTF("sum:%04X, index:%d, count:%d ", rx_sum, rx_index, rx_count);
            if (rx_index == rx_count)
            {
                rx_flag = 6;
            }
            break;

        case 4:
            rx_sum = 0x68 + ch;
            rx_count = ch - 6;
            rx_index = 0;
            rx_flag = 5;
            DEBUG_PRINTF("sum:%04X, count:%d, index:0 ", rx_sum, rx_count);
            break;

        case 3:
            if (ch != rx_prefix[3])
            {
                DEBUG_PRINTF("flag 3 unmatch ");
                rx_flag = 0;
            }
            else
            {
                rx_flag = 4;
            }
            break;

        case 2:
            if (ch != rx_prefix[2])
            {
                DEBUG_PRINTF("flag 2 unmatch ");
                rx_flag = 0;
            }
            else
            {
                rx_flag = 3;
            }
            break;

        case 1:
            if (ch != rx_prefix[1])
            {
                DEBUG_PRINTF("flag 1 unmatch");
                rx_flag = 0;
            }
            else
            {
                rx_flag = 2;
            }
            break;

        case 0:
        default:
            if (ch == rx_prefix[0])
            {
                rx_flag = 1;
            }
            break;
    }
    DEBUG_PRINTF("flag:%d\n", rx_flag);
    if (rx_flag == 0)
    {
        // reset all values
        rx_sum = 0; rx_index = 0; rx_count = 0;
    }
    else if (rx_flag == 9)
    {
        // reset all values
        rx_flag = 0; rx_sum = 0; rx_index = 0; rx_count = 0;
        return 9;
    }
    return rx_flag;
}

/**
 * Read chip response
 * 1. If nothing is received in 100ms, it will return 0
 * 2. If anything is received and run into flags it will keep trying for 100ms for further response
 * 
*/
int chip_read(uint8_t *recv)
{
    /** rx for each rx read, buf to store whole rx */
    uint8_t flag, content_flag = 0, tickdown = 0, *rx = (uint8_t [BUF_SIZE]){}, *rx_p;
    //*buf = (uint8_t [BUF_SIZE]){},  
    int ret, size = 0;

    do
    {
        if ((ret = serial.read(&serial,rx, 255)) > 0)
        {
            rx_p = rx;
            DEBUG_PRINTF("read %d bytes:\n", ret);
            for (uint8_t i = 0; i < ret; i++)
            {
                DEBUG_PRINTF("0x%02x | ", *(rx_p + i));
                // flag check
                flag = flag_check(*(rx_p + i));
                if (flag > 0)
                {
                    tickdown = 10;
                    if ((flag == 5 || flag == 6) && content_flag == 1)
                    {
                        *(recv + (size++)) = *(rx_p + i);
                        content_flag = 1;
                    }
                    if (flag == 5) content_flag = 1;
                }
                else
                {
                    // restart buf;
                    size = 0;
                    content_flag = 0;
                }
            }
        }
        usleep(10000);
    } while (tickdown-- && size < BUF_SIZE && flag > 0);
    if (size > 0)
    {
        for (ret = 0; ret < size; ret++)
        {
            DEBUG_PRINTF("%02X ", *(recv + ret));
        }
        DEBUG_PRINTF("\n");
    }
    
    return size;
}

/* parses a line of intel hex code, stores the data in bytes[] */
/* and the beginning address in addr, and returns a 1 if the */
/* line was valid, or a 0 if an error occured.  The variable */
/* num gets the number of bytes that were stored into bytes[] */

int parse_hex_line(char *theline, int bytes[], int *addr, int *num, int *code)
{
	int sum, len, cksum;
	char *ptr;
	
	*num = 0;
	if (theline[0] != ':') return 0;
	if (strlen(theline) < 11) return 0;
	ptr = theline+1;
	if (!sscanf(ptr, "%02x", &len)) return 0;
	ptr += 2;
	if ( strlen(theline) < (size_t)(11 + (len * 2)) ) return 0;
	if (!sscanf(ptr, "%04x", addr)) return 0;
	ptr += 4;
	  /* printf("Line: length=%d Addr=%d\n", len, *addr); */
	if (!sscanf(ptr, "%02x", code)) return 0;
	ptr += 2;
	sum = (len & 255) + ((*addr >> 8) & 255) + (*addr & 255) + (*code & 255);
	while(*num != len) {
		if (!sscanf(ptr, "%02x", &bytes[*num])) return 0;
		ptr += 2;
		sum += bytes[*num] & 255;
		(*num)++;
		if (*num >= 256) return 0;
	}
	if (!sscanf(ptr, "%02x", &cksum)) return 0;
	if ( ((sum & 255) + (cksum & 255)) & 255 ) return 0; /* checksum error */
	return 1;
}

/* loads an intel hex file into the global memory[] array */
/* filename is a string of the file to be opened */
int load_hex_file(char *filename)
{
	char line[1000];
	FILE *fin;
	int addr, n, status, bytes[256];
	int i, total=0, lineno=1;
	int minaddr=65536, maxaddr=0;

	if (strlen(filename) == 0) {
		return -1;
	}
	fin = fopen(filename, "r");
	if (fin == NULL) {
		printf("   Can't open file '%s' for reading.\n", filename);
		return -1;
	}
	while (!feof(fin) && !ferror(fin)) {
		line[0] = '\0';
		fgets(line, 1000, fin);
		if (line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';
		if (line[strlen(line)-1] == '\r') line[strlen(line)-1] = '\0';
		if (parse_hex_line(line, bytes, &addr, &n, &status)) {
			if (status == 0) {  /* data */
				for(i=0; i<=(n-1); i++) {
					memory[addr] = bytes[i] & 0xFF;
					total++;
					if (addr < minaddr) minaddr = addr;
					if (addr > maxaddr) maxaddr = addr;
					addr++;
				}
			}
			if (status == 1) {  /* end of file */
				fclose(fin);
				printf("   Loaded %d bytes between:", total);
				printf(" %04X to %04X\n", minaddr, maxaddr);
                if (debug)
                {
                    for (int i = minaddr; i <= maxaddr; i++)
                    {
                        printf("%02X ", memory[i]);
                    }
                    printf("\n");
                }
				return maxaddr + 1;
			}
			if (status == 2) {}  /* begin of file */
		} else {
			printf("   Error: '%s', line: %d\n", filename, lineno);
		}
		lineno++;
	}
    return -1;
}
