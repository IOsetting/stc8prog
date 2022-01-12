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
#include "stc8db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define DEFAULTS_PORT   "/dev/ttyUSB0"
#define DEFAULTS_SPEED  115200L

#define FLAG_DEBUG  (1U << 0)
#define FLAG_ERASE  (1U << 1)

static const struct option options[] = {
    {"help",    no_argument,        0,  'h'},
    {"port",    required_argument,  0,  'p'},
    {"speed",   required_argument,  0,  's'},
    {"flash",   required_argument,  0,  'f'},
    {"erase",   no_argument,        0,  'e'},
    {"debug",   no_argument,        0,  'e'},
    {"version", no_argument,        0,  'v'},
    { }, /* NULL */
};

static void usage(void)
{
    printf("Usage: stc8prog [options]...\n");
    printf("  -h, --help            display this message\n");
    printf("  -p, --port <device>   set device path\n");
    printf("  -s, --speed <baud>    set download baudrate\n");
    printf("  -f, --flash <file>    flash chip with data from hex file\n");
    printf("  -e, --erase           erase the entire chip\n");
    printf("  -d, --debug           enable debug output\n");
    printf("  -v, --version         display version information\n");
    printf("\n");
    printf("Baudrate options: \n");
    printf("   4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 500000, 576000,\n");
    printf("   921600, 1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000,\n");
    printf("   4000000\n");
    exit(1);
}

static void version(void)
{
    printf("stc8prog 1.1\n");
    printf("Copyright(c) 2021 IOsetting <iosetting@outlook.com>\n");
    printf("Licensed under the Apache License, Version 2.0\n");
    exit(1);
}

int main(int argc, char *const argv[])
{
    unsigned long flags = 0;
    unsigned int speed = DEFAULTS_SPEED;
    char *file = NULL;
    char *port = DEFAULTS_PORT;
    int optidx, ret, hex_size;
    char arg;
    const stc_model_t *stc_model;
    const stc_protocol_t *stc_protocol;
    uint8_t *recv = (uint8_t [255]){};
    uint16_t chip_code, chip_version, chip_minor_version, chip_stepping;
    uint32_t chip_fosc;

    /** No buffer, disable buffering on stdout  */
    setbuf(stdout, NULL);
    while ((arg = getopt_long(argc, argv, "p:s:f:edhv", options, &optidx)) != -1) {
        switch (arg) {
            case 'p':
                port = optarg;
                break;
            case 's':
                speed = atoi(optarg);
                break;
            case 'f':
                file = optarg;
                break;
            case 'e':
                flags |= FLAG_ERASE;
                break;
            case 'd':
                flags |= FLAG_DEBUG;
                break;
            case 'v':
                version();
                break;
            case 'h': default:
                usage();
        }
    }

    if (argc < 2)
        usage();

    if (flags & FLAG_DEBUG)
        set_debug(true);

    if (file) {
        printf("Loading hex file: ");
        if ((hex_size = load_hex_file(file)) < 0)
        {
            printf("Failed to load hex file\n");
            exit(1);
        }
    }

    printf("Opening port %s: ", port);
    if ((ret = termios_open(port)))
    {
        printf("\e[31mcan not open port\e[0m\n");
        exit(1);
    }
    printf("\e[32mdone\e[0m\n");

    if ((ret = termios_setup(MINBAUD, 8, 1, 'E')))
    {
        printf("\e[31mfailed to communicate chip with baudrate %d\e[0m\n", MINBAUD);
        exit(1);
    }

    printf("Waiting for MCU, please cycle power: ");
    if ((ret = chip_detect(recv)))
    {
        printf("\e[31mfailed to detect chip\e[0m\n");
        exit(1);
    }
    else
    {
        printf("\e[32mdetected\e[0m\n");
    }

    /** chip model */
    chip_code = *(recv + 20);
    chip_code = (chip_code << 8) + *(recv + 21);
    stc_model = model_lookup(chip_code);
    if (stc_model)
    {
        printf("MCU type: \e[32m%s\e[0m\n", stc_model->name);
    }
    else
    {
        printf("MCU type: \e[31munknown code: %04x\e[0m\n", chip_code);
        exit(1);
    }
    /** chip protocol */
    stc_protocol = protocol_lookup(stc_model->protocol);
    if (stc_protocol)
    {
        printf("Protocol: \e[32m%s\e[0m\n", stc_protocol->name);
    }
    else
    {
        printf("Protocol: \e[31munsupported protocol: %04x\e[0m\n", stc_model->protocol);
        exit(1);
    }

    /** f/w version */
    chip_version = *(recv + 17);
    chip_stepping = *(recv + 18);
    chip_minor_version = *(recv + 22);
    printf("F/W version: \e[32m%d.%d.%d%c\e[0m\n", 
        chip_version >> 4, chip_version & 0x0F, chip_minor_version & 0x0F, chip_stepping);

    /** chip fosc */
    chip_fosc = (*(recv + stc_protocol->info_pos_fosc) << 24) 
            + (*(recv + stc_protocol->info_pos_fosc + 1) << 16) 
            + (*(recv + stc_protocol->info_pos_fosc + 2) << 8) 
            + *(recv + stc_protocol->info_pos_fosc + 3);
    printf("IRC frequency(Hz): ");
    if (chip_fosc == 0xffffffff)
    {
        printf("\e[32munadjusted\e[0m\n");
    }
    else
    {
        printf("\e[32m%u\e[0m\n", chip_fosc);
    }

    printf("Switching to \e[32m%d\e[0m baud, chip: ", speed);
    if ((ret = baudrate_set(stc_protocol, speed, recv)))
    {
        printf("failed\n");
        exit(1);
    }
    else
    {
        printf("\e[32mset\e[0m, ");
    }
    
    printf("host: ");
    if ((ret = termios_set_speed(speed)) < 0)
    {
        printf("failed\n");
        exit(1);
    }
    else
    {
        printf("\e[32mset\e[0m, ");
    }

    printf("ping: ");
    if ((ret = baudrate_check(stc_protocol, recv, chip_version)) != 0)
    {
        printf("failed\n");
        exit(1);
    }
    else
    {
        printf("\e[32msucc\e[0m\n");
    }

    if (flags & FLAG_ERASE)
    {
        printf("Erasing chip: ");
        if ((ret = flash_erase(stc_protocol, recv)) != 0)
        {
            printf("failed\n");
            exit(1);
        }
        else
        {
            printf("\e[32msucc\e[0m\n");
        }
    }

    if (file) {
        printf("Writing flash, size %d: ", hex_size);
        if ((ret = flash_write(stc_protocol, hex_size)) != 0)
        {
            printf("failed\n");
        }
        else
        {
            printf("\e[32mdone\e[0m\n");
        }
    }
    return 0;
}