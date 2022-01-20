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

#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

static int ttys;

static speed_t termios_speed[] = {
    B0,       B50,      B75,      B110,     B134,     B150,     B200,     
    B300,     B600,     B1200,    B1800,    B2400,    B4800,    B9600,    
    B19200,   B38400,   B57600,   B115200,  B230400,  B460800,  B500000,
    B576000,  B921600,  B1000000, B1152000, B1500000, B2000000, B2500000, 
    B3000000, B3500000, B4000000,
};

static unsigned int termios_rates[] = {
    0,       50,      75,      110,     134,     150,     200,     
    300,     600,     1200,    1800,    2400,    4800,    9600,    
    19200,   38400,   57600,   115200,  230400,  460800,  500000,
    576000,  921600,  1000000, 1152000, 1500000, 2000000, 2500000, 
    3000000, 3500000, 4000000,
};

int termios_set_speed(unsigned int speed)
{
    struct termios term;
    unsigned int index = 0;
    int retval;

    if ((retval = tcgetattr(ttys, &term)) < 0) 
    {
        return retval;
    }

    while (termios_rates[++index] != speed) 
    {
        if ((index + 1) == ARRAY_SIZE(termios_speed))
        {
            return -1;
        }
    }

    cfsetispeed(&term, termios_speed[index]);
    cfsetospeed(&term, termios_speed[index]);

    tcflush(ttys, TCIOFLUSH);
    return tcsetattr(ttys, TCSANOW, &term);
}

/**
 * CSIZE:  Character size mask.  Values are CS5, CS6, CS7, or CS8
 * CSTOPB: Set two stop bits, rather than one.
 * CLOCAL: Ignore modem control lines.
 * CREAD:  Enable receiver.
 * INPCK:  Enable input parity checking
 * PARENB: Enable parity generation on output and parity checking for input.
 * ICANON: Enable canonical mode (described below)
 * ECHO:   Echo input characters.
 * ECHOE:  If ICANON is also set, the ERASE character erases the preceding input character, and WERASE erases the preceding word.
 * ISIG:   When any of the characters INTR, QUIT, SUSP, or DSUSP are received, generate the corresponding signal.
 * 
 * OPOST:  Enable implementation-defined output processing.
 * 
 * ICRNL:  Translate carriage return to newline on input (unless IGNCR is set).
 * XON:    Enable XON/XOFF flow control on output.
*/
int termios_setup(unsigned int speed, int databits, int stopbits, char parity)
{
    struct termios term;
    int ret;

    if ((ret = termios_set_speed(speed)) < 0)
    {
        return ret;
    }

    if ((ret = tcgetattr(ttys, &term)) < 0) 
    {
        return ret;
    }

    term.c_cflag |= (CLOCAL | CREAD);

    term.c_cflag &= ~CSIZE;
    if (databits == 7)
        term.c_cflag |= CS7;
    else
        term.c_cflag |= CS8;

    if (stopbits == 2)
        term.c_cflag |= CSTOPB;
    else
        term.c_cflag &= ~CSTOPB;

    switch (parity) {
        case 'N': case 'n':
            term.c_cflag &= ~PARENB;
            term.c_iflag &= ~INPCK;
            break;
        case 'O': case 'o':
            term.c_cflag |= (PARODD | PARENB);
            term.c_iflag |= INPCK;
            break;
        case 'E': case 'e':
            term.c_cflag |= PARENB;
            term.c_cflag &= ~PARODD;
            term.c_iflag |= INPCK;
            break;
        case 'S': case 's':
            term.c_cflag &= ~PARENB;
            term.c_cflag &= ~CSTOPB;
            term.c_iflag |= INPCK;
            break;
    }

    /* set raw input, not using cfmakeraw(&term) */

    /* local modes - clear giving: echoing off, canonical off (no erase with
        backspace, ^U,...),  no extended functions, no signal chars (^Z,^C) */
    term.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG | ECHONL);
    /* output modes - clear giving: no post processing such as NL to CR+NL */
    term.c_oflag &= ~(OPOST);
    /* input modes - clear indicated ones giving: no break, no CR to NL,
        no parity check, no strip char, no start/stop output (sic) control */
    term.c_iflag &= ~(BRKINT | ICRNL | ISTRIP | IXON | IGNBRK | PARMRK | INLCR | IGNCR);

    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0; /* immediate - anything       */

    tcflush(ttys, TCIOFLUSH);
    if ((ret = tcsetattr(ttys, TCSANOW, &term)) < 0) 
    {
        return ret;
    }
    return 0;
}

int termios_rts(bool enable)
{
    unsigned int state;
    int ret;

    if ((ret = ioctl(ttys, TIOCMGET, &state)) < 0)
        return ret;

    if (enable)
        state |= TIOCM_RTS;
    else
        state &= ~TIOCM_RTS;

    return ioctl(ttys, TIOCMSET, &state);
}

int termios_flush(void)
{
    return tcflush(ttys, TCIFLUSH);
}

int termios_read(void *data, unsigned long len)
{
    int ret = read(ttys, data, len);
    tcflush(ttys, TCIFLUSH);
    return ret;
}

int termios_write(const void *data, unsigned long len)
{
    int ret = write(ttys, data, len);
    tcflush(ttys, TCOFLUSH);
    return ret;
}

int termios_open(char *path)
{
    if (ttys)
        return -EALREADY;

    ttys = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
    fcntl(ttys, F_SETFL, O_NDELAY);

    return ttys < 0 ? ttys : 0;
}
