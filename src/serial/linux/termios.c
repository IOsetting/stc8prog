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
#include <string.h>
#include "userial.h"

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x) 
#define unlikely(x)
#endif

/**
 * @struct the serial port of the Linux.
 * inherited from generic
 */
typedef struct linux_serial {
    /* generic data */
    userial_t generic; 
    /* linux-specific data */
    struct {
        int ttys;
    } linux_specific;
} linux_serial_t;

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

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

/*** serial access funtion table ***/

/***
 * @brief open serial port
 * @param this  - [out] serial port instance
 * @param path  - [in] serial port device path  
 * 
 * @return      - 0 on success, error code otherwise
 */ 
int32_t termios_ctor(linux_serial_t * restrict const this,
                     char *path)
{
    if (unlikely(SERIAL_PORT_INIT_MAGIC == this->generic.initiated)) {
        /* mark that sereil port ready to run anyway */
        return -EALREADY;
    }

    const int ttys = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
    fcntl(ttys, F_SETFL, O_NDELAY);

    const bool success = 0 < ttys;
    if(likely(success)){
        this->linux_specific.ttys = ttys;
        this->generic.initiated = SERIAL_PORT_INIT_MAGIC;
        (void)strncpy((char*)this->generic.name, path, sizeof(this->generic.name) - 1);
        this->generic.name[sizeof(this->generic.name) - 1] = '\0';
        return 0;
    }else{
        return ttys;
    }
}

/***
 * @brief close serial port
 * @param this  - [out] serial port instance
 * 
 * @return      - 0 on success, error code otherwise
 */ 
int32_t termios_dtor(linux_serial_t * restrict const this)
{
    if(likely(SERIAL_PORT_INIT_MAGIC == this->generic.initiated)) {
        this->generic.initiated = 0;
        const int res = close(this->linux_specific.ttys);
        return 0;
    }
    return -ENODEV;
}

/***
 * @brief set speed for serial port
 * @param this   - [inout] serial port instance
 * @param speed  - [in] serial port speed to set  
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
int32_t termios_speed_set(linux_serial_t * restrict const this,
                          uint32_t speed)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }

    struct termios term;
    unsigned int index = 0;
    int retval;

    if ((retval = tcgetattr(this->linux_specific.ttys, &term)) < 0) 
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

    tcflush(this->linux_specific.ttys, TCIOFLUSH);
    retval = tcsetattr(this->linux_specific.ttys, TCSANOW, &term);
    if(likely(0 == retval)){
        this->generic.speed = speed;
    }
    return retval;
}

/***
 * @brief flush serial port
 * @param this   - [inout] serial port instance
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
int32_t termios_flush(linux_serial_t * restrict const this)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }

    return tcflush(this->linux_specific.ttys, TCIFLUSH);
}

/***
 * @brief setup serial port
 * @param this      - [inout] serial port instance
 * @param speed     - [in] serial port speed
 * @param databits  - [in] data bits count
 * @param stopbits  - [in] stop bits count
 * @param parity    - [in] parity
 * 
 * @return          - 0 on success, error code otherwise 
 */ 
int32_t termios_setup(linux_serial_t * restrict const this,
                      uint32_t speed,
                      uint8_t databits,
                      uint8_t stopbits,
                      userial_parity_t parity)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }

    struct termios term;
    int ret;

    const int32_t speed_res = termios_speed_set(this, speed);
    if(unlikely(0 > speed_res)){
        return speed_res;
    }

    const int getattr_res = tcgetattr(this->linux_specific.ttys, &term); 
    if (unlikely(0 > getattr_res)) {
        return getattr_res;
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
        case USERIAL_PARITY_NONE:
            term.c_cflag &= ~PARENB;
            term.c_iflag &= ~INPCK;
            break;
        case USERIAL_PARITY_ODD:
            term.c_cflag |= (PARODD | PARENB);
            term.c_iflag |= INPCK;
            break;
        case USERIAL_PARITY_EVEN:
            term.c_cflag |= PARENB;
            term.c_cflag &= ~PARODD;
            term.c_iflag |= INPCK;
            break;
        case USERIAL_PARITY_SPACE:
            term.c_cflag &= ~PARENB;
            term.c_cflag &= ~CSTOPB;
            term.c_iflag |= INPCK;
            break;
        case USERIAL_PARITY_MARK:
            /* todo: need to be checked */
            term.c_cflag &= ~PARENB;
            term.c_cflag &= ~CSTOPB;
            term.c_cflag |= PARODD;
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

    tcflush(this->linux_specific.ttys, TCIOFLUSH);

    const int apply_res = tcsetattr(this->linux_specific.ttys, TCSANOW, &term);
    if(unlikely(0 > apply_res)){
        return apply_res;
    }
    return 0;
}

/***
 * @brief set level on rts serial port line
 * @param this   - [inout] serial port instance
 * @param level  - [in] logical level on rts line 
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
int32_t termios_rts(linux_serial_t * restrict const this,
                    bool level)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }
    unsigned int state;
    int ret;

    const int state_get_res = ioctl(this->linux_specific.ttys, TIOCMGET, &state);
    if(unlikely(0 > state_get_res)){
        return state_get_res;
    }

    if(level){
        state |= TIOCM_RTS;
    }else{
        state &= ~TIOCM_RTS;
    }

    const int state_set_res = ioctl(this->linux_specific.ttys, TIOCMSET, &state);
    if(unlikely(0 > state_set_res)){
        return state_set_res;
    }
    return 0;
}

/***
 * @brief set level on dtr serial port line
 * @param this   - [inout] serial port instance
 * @param level  - [in] logical level on dtr line 
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
int32_t termios_dtr(linux_serial_t * restrict const this,
                    bool level)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }
    unsigned int state;
    int ret;

    const int state_get_res = ioctl(this->linux_specific.ttys, TIOCMGET, &state);
    if(unlikely(0 > state_get_res)){
        return state_get_res;
    }

    if(level){
        state |= TIOCM_DTR;
    }else{
        state &= ~TIOCM_DTR;
    }

    const int state_set_res = ioctl(this->linux_specific.ttys, TIOCMSET, &state);
    if(unlikely(0 > state_set_res)){
        return state_set_res;
    }
    return 0;
}

/***
 * @brief read data from serial port
 * @param this      - [inout] serial port instance
 * @param dst       - [out] serial port read destination
 * @param dst_siz   - [in] destination size
 * 
 * @return          - read data count on success, error code otherwise 
 */ 
int32_t termios_read(linux_serial_t * restrict const this,
                     uint8_t * restrict const dst,
                     const uint32_t dst_siz)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }

    const int ret = read(this->linux_specific.ttys, dst, dst_siz);

    /***
     * todo: seems read flush can cause problems sometimes,
     * need to be checked
     * tcflush(ttys, TCIFLUSH);
     */ 
    return ret;
}

/***
 * @brief write data to serial port
 * @param this      - [inout] serial port instance
 * @param dst       - [out] serial port read destination
 * @param dst_siz   - [in] destination size
 * 
 * @return          - write data count on success, error code otherwise 
 */ 
int32_t termios_write(linux_serial_t * restrict const this,
                      const uint8_t * restrict const src,
                      const uint32_t src_siz)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }

    const int ret = write(this->linux_specific.ttys, src, src_siz);
    /***
     * todo: seems read flush can cause problems sometimes,
     * need to be checked
     * tcflush(ttys, TCOFLUSH);
     */ 
    return ret;
}

    /*** serial port instance ***/

linux_serial_t serial = {
    /* generic */
    .generic = {
        .ctor = (userial_ctor_t)termios_ctor,  
        .dtor = (userial_dtor_t)termios_dtor, 
        .speed_set = (userial_speed_set_t)termios_speed_set,
        .flush = (userial_flush_t)termios_flush,
        .setup = (userial_setup_t)termios_setup, 
        .rts_set = (userial_rts_set_t)termios_rts,
        .dtr_set = (userial_dtr_set_t)termios_dtr,
        .read = (userial_read_t) termios_read,
        .write = (userial_write_t)termios_write, 

        .initiated = 0,
    },

    /* linux-specific */    
    .linux_specific.ttys = 0,  
};


