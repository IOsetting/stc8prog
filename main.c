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
    printf("stc8prog 1.0\n");
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
    int optidx, ret;
    char arg;
    uint8_t *buff = (uint8_t [255]){};

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

    if ((ret = termios_open(port)))
    {
        printf("** Failed to open port %s\n", port);
        exit(1);
    }

    if ((ret = termios_setup(MINBAUD, 8, 1, 'E')))
    {
        printf("** Failed to communicate chip with baudrate %d\n", MINBAUD);
        exit(1);
    }

    if ((ret = entry_detect(buff)))
    {
        printf("** Failed to detect chip\n");
        exit(1);
    }
    else
    {
        printf("\e[32mdone\e[0m\n");
    }

    printf("Switching to \e[32m%d\e[0m baud, set chip: ", speed);
    if ((ret = baudrate_set(speed, buff)))
    {
        printf("failed\n");
        exit(1);
    }
    else
    {
        printf("\e[32mdone\e[0m, ");
    }
    
    printf("set host: ");
    if ((ret = termios_setup(speed, 8, 1, 'E')) < 0)
    {
        printf("failed\n");
        exit(1);
    }
    else
    {
        printf("\e[32mdone\e[0m, ");
    }

    printf("testing: ");
    if ((ret = baudrate_check(buff)) < 0)
    {
        printf("failed\n");
        exit(1);
    }
    else
    {
        printf("\e[32mdone\e[0m\n");
    }

    if (flags & FLAG_ERASE)
    {
        printf("Erasing chip: ");
        if ((ret = erase_flash(buff)) < 0)
        {
            printf("failed\n");
            exit(1);
        }
        else
        {
            printf("\e[32mdone\e[0m\n");
        }
    }

    if (file) {
        printf("Loading hex file: ");
        if ((ret = load_hex_file(file)) < 0)
        {
            printf("Failed to load hex file\n");
            exit(1);
        }
        printf("Writing flash, size %d: ", ret);
        if ((ret = write_flash(ret)) < 0)
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