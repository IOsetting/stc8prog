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

static uint8_t rx_prefix[] = {0x46, 0xb9, 0x68, 0x00};
uint8_t debug = 0, memory[65536];

void set_debug(uint8_t val)
{
    debug = val;
}

int write_flash(unsigned int len)
{
    uint8_t 
        *buff = (uint8_t [255]){},
        *recv = (uint8_t [255]){},
        arg[255] = {};
    unsigned int addr, offset;
    uint8_t cnt, count;
    int ret;

    arg[0] = 0x22;
    arg[3] = 0x5a;
    arg[4] = 0xa5;

    addr = 0;
    offset = 5;

    while (addr < len)
    {
        arg[1] = HIBYTE(addr);
        arg[2] = LOBYTE(addr);
        printf("0x%0x ", addr);

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
            ret = chip_read(buff , 255);
            if (ret <= 0)
            {
                DEBUG_PRINTF("read nothing\n");
                continue;
            }
            ret = chip_read_verify(buff, ret, recv);
            if (ret > 0)
            {
                DEBUG_PRINTF("write flash received: %d\n", ret);
                if (*recv == 0x02 && *(recv + 1) == 'T')
                {
                    printf("- ");
                    arg[0] = 0x02;
                    break;
                }
                else
                {
                    printf("unmatch 'T'\n");
                    return -1;
                }
            }
        }
    }
    return 0;
}

int erase_flash(uint8_t *buff)
{
    unsigned int count, ret;
    uint8_t *recv = (uint8_t [255]){};
    uint8_t arg[10] = {};
    arg[0] = 0x03;
    arg[1] = 0x00;
    arg[2] = 0x00;
    arg[3] = 0x5a;
    arg[4] = 0xa5;

    chip_write(arg, 5);
    for (count = 0; count < 10; ++count)
    {
        ret = chip_read(buff , 255);
        if (ret <= 0)
        {
            DEBUG_PRINTF("read nothing\n");
            continue;
        }
        ret = chip_read_verify(buff, ret, recv);
        if (ret > 0)
        {
            DEBUG_PRINTF("erase flash received: %d\n", ret);
            if (*recv == 0x03)
            {
                return 0;
            }
            else
            {
                printf("unmatch 0x05\n");
                return 1;
            }
        }
    }
    return 1;
}

int baudrate_check(uint8_t *buff)
{
    usleep(100000);
    unsigned int count, ret;
    uint8_t *recv = (uint8_t [255]){};
    uint8_t arg[10] = {};
    arg[0] = 0x05;
    arg[1] = 0x00;
    arg[2] = 0x00;
    arg[3] = 0x5a;
    arg[4] = 0xa5;

    for (count = 0; count < 10; ++count) 
    {
        chip_write(arg, 5);
        ret = chip_read(buff , 255);
        if (ret <= 0)
        {
            continue;
        }
        ret = chip_read_verify(buff, ret, recv);
        if (ret > 0)
        {
            DEBUG_PRINTF("baudrate check received: %d\n", ret);
            if (*recv == 0x05)
            {
                return 0;
            }
            else
            {
                printf("unmatch 0x05\n");
                return 1;
            }
        }
    }
    return 1;
}

int baudrate_set(unsigned int speed, uint8_t *buff)
{
    unsigned int count, ret;
    uint8_t *recv = (uint8_t [255]){};
    uint8_t arg[255] = {};
    arg[0] = 0x01;
    arg[1] = *(buff + 4);
    arg[2] = 0x40;
    arg[3] = HIBYTE(RL(speed));
    arg[4] = LOBYTE(RL(speed));
    arg[5] = 0x00;
    arg[6] = 0x00;
    arg[7] = 0x97;

    for (count = 0; count < 10; ++count) 
    {
        chip_write(arg, 8);
        ret = chip_read(buff , 255);
        if (ret <= 0)
        {
            continue;
        }
        ret = chip_read_verify(buff, ret, recv);
        if (ret > 0)
        {
            DEBUG_PRINTF("baudrate set received: %d\n", ret);
            if (*recv == 0x01)
            {
                return 0;
            }
            else
            {
                printf("unmatch 0x01\n");
                return 1;
            }
        }
    }
    return 1;
}

int entry_detect(uint8_t *buff)
{
    uint8_t *recv = (uint8_t [255]){};
    unsigned int count;
    int ret;

    printf("Waiting for MCU, please cycle power: ");
    for (count = 0; count < 100; count++) 
    {
        termios_write(&(uint8_t){0x7F}, 1);
        if ((ret = chip_read(buff , 255)) <= 0)
        {
            printf(".");
            continue;
        }
        if ((ret = chip_read_verify(buff, ret, recv)) > 0)
        {
            if (*recv == 0x50)
            {
                return 0;
            }
            else
            {
                printf("entry_detect read unmatched\n");
                return -1;
            }
        }
    }
    printf("timeout\n");
    return -1;
}

int chip_write(uint8_t *buff, uint8_t len)
{
    uint16_t sum;
    uint8_t i;
    DEBUG_PRINTF("TX: ");
    termios_write(&(uint8_t){0x46}, 1);
    termios_write(&(uint8_t){0xb9}, 1);
    termios_write(&(uint8_t){0x6a}, 1);
    termios_write(&(uint8_t){0x00}, 1);
    sum = len + 6 + 0x6a;
    termios_write(&(uint8_t){len + 6}, 1);
    for (i = 0; i < len; i++)
    {
        sum += *(buff + i);
        termios_write(&(uint8_t){*(buff + i)}, 1);
    }
    termios_write(&(uint8_t){HIBYTE(sum)}, 1);
    termios_write(&(uint8_t){LOBYTE(sum)}, 1);
    termios_write(&(uint8_t){0x16}, 1);
    DEBUG_PRINTF("\n");
    return 0;
}

int chip_read_verify(uint8_t *rx, uint8_t len, uint8_t *content)
{
    uint16_t RecvSum;
    uint8_t tmp, RecvCount, RecvIndex;
    int flag = 0;

    while(len-- > 0)
    {
        tmp = *rx++;
        DEBUG_PRINTF("0x%02X ", tmp);
        switch (flag)
        {
            case 8:
                if (tmp != 0x16)
                {
                    DEBUG_PRINTF("end byte unmatched\n");
                    return -1;
                }
                return RecvIndex;
            case 7:
                DEBUG_PRINTF("sum check: 0x%02X ", LOBYTE(RecvSum));
                if (tmp != LOBYTE(RecvSum))
                {
                    DEBUG_PRINTF("low byte of sum unmatched\n");
                    return -1;
                }
                DEBUG_PRINTF("\n");
                flag = 8;
                break;
            case 6:
                DEBUG_PRINTF("sum check: 0x%02X ", HIBYTE(RecvSum));
                if (tmp != HIBYTE(RecvSum))
                {
                    DEBUG_PRINTF("high byte of sum unmatched\n");
                    return -1;
                }
                DEBUG_PRINTF("\n");
                flag = 7;
                break;
            case 5:
                RecvSum += tmp;
                content[RecvIndex++] = tmp;
                DEBUG_PRINTF("0x%02X, RecvSum:%04X, RecvCount:%d, RecvIndex:%d\n", tmp, RecvSum, RecvCount, RecvIndex);
                if (RecvIndex == RecvCount)
                {
                    flag = 6;
                }
                break;
            case 4:
                RecvSum = 0x68 + tmp;
                RecvCount = tmp - 6;
                RecvIndex = 0;
                flag = 5;
                DEBUG_PRINTF("0x%02X, RecvSum:%04X, RecvCount:%d, RecvIndex:0, #5\n", tmp, RecvSum, RecvCount);
                break;
            case 3:
                if (tmp != rx_prefix[3])
                {
                    DEBUG_PRINTF("flag 3 unmatch\n");
                    return -1;
                }
                flag = 4;
                break;
            case 2:
                if (tmp != rx_prefix[2])
                {
                    DEBUG_PRINTF("flag 2 unmatch\n");
                    return -1;
                }
                flag = 3;
                break;
            case 1:
                if (tmp != rx_prefix[1])
                {
                    DEBUG_PRINTF("flag 1 unmatch\n");
                    return -1;
                }
                flag = 2;
                break;
            case 0:
            default:
                if (tmp != rx_prefix[0])
                {
                    return -1;
                }
                flag = 1;
        }
    }
    return 0;
}

int chip_read(uint8_t *buff, uint8_t len)
{
    uint8_t *rx = (uint8_t [255]){}, *rx_p;
    int ret, size = 0;
    while((ret = termios_read(rx, 255)) > 0)
    {
        rx_p = rx;
        if (ret > 0)
        {
            DEBUG_PRINTF("read %d bytes: ", ret);
            for (uint8_t i = 0; i < ret; i++)
            {
                DEBUG_PRINTF("%02x | ", *(rx_p + i));
            }
            DEBUG_PRINTF("\n");
            while (ret-- > 0)
            {
                *buff++ = *rx_p++;
                size++;
                if (size == len)
                {
                    return size;
                }
            }
        }
    }
    if (ret < 0)
    {
        printf("termios_read error...\n");
        return ret;
    }
    return size;
}

/* parses a line of intel hex code, stores the data in bytes[] */
/* and the beginning address in addr, and returns a 1 if the */
/* line was valid, or a 0 if an error occured.  The variable */
/* num gets the number of bytes that were stored into bytes[] */

int parse_hex_line(theline, bytes, addr, num, code)
char *theline;
int *addr, *num, *code, bytes[];
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
