// Copyright 2021-2022 IOsetting <iosetting@outlook.com>,
//                     Ivan Nalogin <egan.fryazino@gmail.com>, 
//                     Alexey Evtyushkin <earvest@gmail.com>
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
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <string.h>
#include <unistd.h>
#include "userial.h"

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x) 
#define unlikely(x)
#endif

/**
 * @struct the serial port of the win32
 * inherited from generic
 */
typedef struct win32_serial {
    /* generic data */
    userial_t generic; 
    /* win32-specific data */
    struct {
        HANDLE ttys;
        OVERLAPPED overlapped_read;
        OVERLAPPED overlapped_write;
    } win32_specific;
} win32_serial_t;

/*** serial access funtion table ***/

/***
 * @brief open serial port
 * @param this  - [out] serial port instance
 * @param path  - [in] serial port device path  
 * 
 * @return      - 0 on success, error code otherwise
 */ 
int32_t com_ctor(win32_serial_t * restrict const this,
                 char *path)
{
    if (unlikely(SERIAL_PORT_INIT_MAGIC == this->generic.initiated)) {
        /* mark that sereil port ready to run anyway */
        return -EALREADY;
    }

    const HANDLE hSerial = CreateFile(path,
		                              GENERIC_READ | GENERIC_WRITE,
		                              0,
		                              NULL,
		                              OPEN_EXISTING,
		                              FILE_FLAG_OVERLAPPED,
		                              NULL);

    if(INVALID_HANDLE_VALUE != hSerial){
        this->generic.initiated = SERIAL_PORT_INIT_MAGIC;
        this->win32_specific.ttys =  hSerial;
        (void)strncpy((char*)this->generic.name, path, sizeof(this->generic.name) - 1);
        this->generic.name[sizeof(this->generic.name) - 1] = '\0';
        return 0;
    }else{
        return -EBADF;
    }
}

/***
 * @brief close serial port
 * @param this  - [out] serial port instance
 * 
 * @return      - 0 on success, error code otherwise
 */ 
int32_t com_dtor(win32_serial_t * restrict const this)
{
    if(likely(SERIAL_PORT_INIT_MAGIC == this->generic.initiated)) {
        this->generic.initiated = 0;
        const bool res = CloseHandle(this->win32_specific.ttys);
        return res ? 0: -EBADF;
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
int32_t com_speed_set(win32_serial_t * restrict const this,
                      uint32_t speed)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }

	DCB dcbSerialParams = {0};
	dcbSerialParams.DCBlength=sizeof(dcbSerialParams);
	if (!GetCommState(this->win32_specific.ttys, &dcbSerialParams)) {
		return -EIO;
	}

	if( dcbSerialParams.BaudRate != speed ) {
		dcbSerialParams.BaudRate = speed;
		if(!SetCommState(this->win32_specific.ttys, &dcbSerialParams)){
			return -EIO;
		}
	}

    this->generic.speed = speed;
    return 0;
}

/***
 * @brief flush serial port
 * @param this   - [inout] serial port instance
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
int32_t com_flush(win32_serial_t * restrict const this)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }
    /* todo: in pyseral used another flush method */
    const bool res = PurgeComm(this->win32_specific.ttys, PURGE_RXCLEAR | PURGE_RXABORT);
    return (res ? 0: -EIO);
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
int32_t com_setup(win32_serial_t * restrict const this,
                  uint32_t speed,
                  uint8_t databits,
                  uint8_t stopbits,
                  userial_parity_t parity)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }

    const bool setup_res = SetupComm (this->win32_specific.ttys, 4096, 4096); 
    if(unlikely(!setup_res)){
        printf("com_setup: Cannot setup buffer size");
        return -EIO;
    }

	this->win32_specific.overlapped_read.Internal = 0;
	this->win32_specific.overlapped_read.InternalHigh = 0;
	this->win32_specific.overlapped_read.Offset = 0;
	this->win32_specific.overlapped_read.OffsetHigh = 0;
	this->win32_specific.overlapped_write.Internal = 0;
	this->win32_specific.overlapped_write.InternalHigh = 0;
	this->win32_specific.overlapped_write.Offset = 0;
	this->win32_specific.overlapped_write.OffsetHigh = 0;
	
	this->win32_specific.overlapped_read.hEvent = CreateEvent(NULL, 1, 0, NULL);
	this->win32_specific.overlapped_write.hEvent = CreateEvent(NULL, 0, 0, NULL);

	DCB dcbSerialParams = {0};

	dcbSerialParams.DCBlength=sizeof(dcbSerialParams);

	if (!GetCommState(this->win32_specific.ttys, &dcbSerialParams)) {
		printf("com_setup: Cannot get com state\n");
		return -EIO;
	}

	dcbSerialParams.fBinary = 1;
	dcbSerialParams.fAbortOnError = 0;
	dcbSerialParams.wReserved = 0;
	dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
	dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
	dcbSerialParams.fParity = false;
	dcbSerialParams.fInX = 0;
	dcbSerialParams.fOutX = 0;
	dcbSerialParams.XonChar = 0x11;
	dcbSerialParams.XoffChar = 0x13;
	dcbSerialParams.ErrorChar = 0;
	dcbSerialParams.fErrorChar = 0;
	dcbSerialParams.fOutxCtsFlow = 0;
	dcbSerialParams.fOutxDsrFlow = 0;
	dcbSerialParams.XonLim = 0;
	dcbSerialParams.XoffLim = 0;
	dcbSerialParams.fNull = 0;

	dcbSerialParams.BaudRate=speed;
	dcbSerialParams.ByteSize=databits;
	dcbSerialParams.StopBits= (1 == stopbits) ? ONESTOPBIT: TWOSTOPBITS;
    
    switch (parity) {
        case USERIAL_PARITY_NONE:
            dcbSerialParams.Parity = NOPARITY;
            break;
        case USERIAL_PARITY_ODD:
            dcbSerialParams.Parity = ODDPARITY;
            break;
        case USERIAL_PARITY_EVEN:
            dcbSerialParams.Parity = EVENPARITY;
            break;
        case USERIAL_PARITY_SPACE:
            dcbSerialParams.Parity = SPACEPARITY;
            break;
        case USERIAL_PARITY_MARK:
            dcbSerialParams.Parity = MARKPARITY;
            break;
    }
	if(!SetCommState(this->win32_specific.ttys, &dcbSerialParams)){
		printf("com_setup: Cannot set com state\n");
		return -EIO;
	}

	COMMTIMEOUTS timeouts={0};
	timeouts.ReadIntervalTimeout=MAXDWORD;
	timeouts.ReadTotalTimeoutConstant=0;
    timeouts.ReadTotalTimeoutMultiplier=0;
    timeouts.WriteTotalTimeoutConstant=0;
    timeouts.WriteTotalTimeoutMultiplier=0;

	if(!SetCommTimeouts(this->win32_specific.ttys, &timeouts)){
		printf("com_setup: Cannot set timeouts\n");
		return -EIO;
	}
	
	const bool set_mask_res = SetCommMask(this->win32_specific.ttys, EV_ERR);
    if(unlikely(!set_mask_res)){
        return -EIO; 
    }

	const bool purge_res = PurgeComm(this->win32_specific.ttys, PURGE_RXCLEAR | PURGE_RXABORT);
    if(unlikely(!purge_res)){
        return -EIO; 
    }
    /* Windows happy only when rts is set */
    const bool rts_res = EscapeCommFunction(this->win32_specific.ttys, SETRTS);
    if(unlikely(!rts_res)){
        return -EIO;
    }

    this->generic.databits = databits;
    this->generic.stopbits = stopbits;
    this->generic.parity = parity;
    return 0;
}

/***
 * @brief set level on rts serial port line
 * @param this   - [inout] serial port instance
 * @param level  - [in] logical level on rts line 
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
int32_t com_rts(win32_serial_t * restrict const this,
                bool level)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }

    const bool res = EscapeCommFunction(this->win32_specific.ttys, level ? SETRTS : CLRRTS);
    return (res ? 0: -EIO);
}

/***
 * @brief set level on dtr serial port line
 * @param this   - [inout] serial port instance
 * @param level  - [in] logical level on dtr line 
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
int32_t com_dtr(win32_serial_t * restrict const this,
                bool level)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }
    const bool res = EscapeCommFunction(this->win32_specific.ttys, level ? SETDTR : CLRDTR);
    return (res ? 0: -EIO);
}

/***
 * @brief read data from serial port
 * @param this      - [inout] serial port instance
 * @param dst       - [out] serial port read destination
 * @param dst_siz   - [in] destination size
 * 
 * @return          - read data count on success, error code otherwise 
 */ 
int32_t com_read(win32_serial_t * restrict const this,
                 uint8_t * restrict const dst,
                 const uint32_t dst_siz)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }

	DWORD flags;
	COMSTAT comstat;
    DWORD dwBytesRead = 0;

	const bool reset_res = ResetEvent(this->win32_specific.overlapped_read.hEvent);
    if(unlikely(!reset_res)){
        return -EIO;
    }

	const bool clear_res = ClearCommError(this->win32_specific.ttys,
                                          &flags,
                                          &comstat);
    if(unlikely(!clear_res)){
        return -EIO;
    }                                 

	const bool read_ok = ReadFile(this->win32_specific.ttys,
                                  dst,
                                  dst_siz,
                                  &dwBytesRead,
                                  &this->win32_specific.overlapped_read);

	DWORD error_id = GetLastError();

    if(unlikely((!read_ok) && (ERROR_SUCCESS != error_id) && (ERROR_IO_PENDING != error_id))){
		printf("com_read: cannot read %ld\n", GetLastError());
        return -1;
    }

	const bool get_res = GetOverlappedResult(this->win32_specific.ttys, &this->win32_specific.overlapped_read, &dwBytesRead, true);
    if(unlikely(!get_res)){
        return -EIO;
    }
    /* print received data for debug purposes
	 * if( dwBytesRead > 0 ) {
	 *	printf("com_read: read %lu bytes\n", dwBytesRead);
     *
	 *	for(DWORD sel=0; sel<dwBytesRead; sel++) {
	 *		printf(" %02X", *((unsigned char *)dst + (int)sel));
	 *	}
	 *	printf("\n");
	 * }
     */

    return dwBytesRead;
}

/***
 * @brief write data to serial port
 * @param this      - [inout] serial port instance
 * @param dst       - [out] serial port read destination
 * @param dst_siz   - [in] destination size
 * 
 * @return          - write data count on success, error code otherwise 
 */ 
int32_t com_write(win32_serial_t * restrict const this,
                  const uint8_t * restrict const src,
                  const uint32_t src_siz)
{
    if(unlikely(SERIAL_PORT_INIT_MAGIC != this->generic.initiated)) {
        return -ENODEV;
    }

	DWORD dwBytesWr = 0;

	const bool write_ok = WriteFile(this->win32_specific.ttys, src, src_siz, &dwBytesWr, &this->win32_specific.overlapped_write);
	const DWORD error_id = GetLastError();

	if(unlikely((!write_ok) && (error_id != ERROR_SUCCESS) && (error_id != ERROR_IO_PENDING))){
		printf("com_write: cannot write\n");
		return -1;
	}

	const bool get_res = GetOverlappedResult(this->win32_specific.ttys, &this->win32_specific.overlapped_write, &dwBytesWr, true);
    if(unlikely(!get_res)){
        return -EIO;
    }

    /** print transmitted data length for debug purposes 
	 * if( dwBytesWr > 0 ) {
	 *	printf("com_write: %lu bytes written\n", dwBytesWr);
	 *}
     */

	return dwBytesWr;
}

    /*** serial port instance ***/

win32_serial_t serial = {
    /* generic */
    .generic = {
        .ctor = (userial_ctor_t)com_ctor,  
        .dtor = (userial_dtor_t)com_dtor, 
        .speed_set = (userial_speed_set_t)com_speed_set,
        .flush = (userial_flush_t)com_flush,
        .setup = (userial_setup_t)com_setup, 
        .rts_set = (userial_rts_set_t)com_rts,
        .dtr_set = (userial_dtr_set_t)com_dtr,
        .read = (userial_read_t)com_read,
        .write = (userial_write_t)com_write, 

        .initiated = 0,
    },

    /* windows-specific */    
};


