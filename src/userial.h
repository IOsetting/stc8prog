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

#ifndef USERIAL_H
#define USERIAL_H
#include "stdint.h"
#include "stdbool.h"

/* maximum port path length */
#define SERIAL_PORT_PATH_MAX        64
/* if initiated field contain init magic value, than port is initiated */
#define SERIAL_PORT_INIT_MAGIC      0x12345678

typedef enum __attribute__((packed)){
    USERIAL_PARITY_NONE = 0,
    USERIAL_PARITY_ODD,
    USERIAL_PARITY_EVEN,
    USERIAL_PARITY_MARK,
    USERIAL_PARITY_SPACE,
} userial_parity_t;


/* forward declaration */
struct userial;

/***
 * @brief open serial port
 * @param this  - [out] serial port instance
 * @param path  - [in] serial port device path  
 * 
 * @return      - 0 on success, error code otherwise
 */ 
typedef int32_t (*userial_ctor_t)(struct userial * restrict const this,
                               const char * path);

/***
 * @brief close serial port
 * @param this  - [out] serial port instance
 * 
 * @return      - 0 on success, error code otherwise
 */ 
typedef int32_t (*userial_dtor_t)(struct userial * restrict const this);

/***
 * @brief set speed for serial port
 * @param this   - [inout] serial port instance
 * @param speed  - [in] serial port speed to set  
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
typedef int32_t (*userial_speed_set_t)(struct userial * restrict const this,
                                       const uint32_t speed);

/***
 * @brief flush serial port
 * @param this   - [inout] serial port instance
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
typedef int32_t (*userial_flush_t)(struct userial * restrict const this);

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
typedef int32_t (*userial_setup_t)(struct userial * restrict const this,
                                   const uint32_t speed,
                                   const uint8_t databits,
                                   const uint8_t stopbits,
                                   const userial_parity_t parity);

/***
 * @brief set level on rts serial port line
 * @param this   - [inout] serial port instance
 * @param level  - [in] logical level on rts line 
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
typedef int32_t (*userial_rts_set_t)(struct userial * restrict const this,
                                     const bool level);

/***
 * @brief set level on dtr serial port line
 * @param this   - [inout] serial port instance
 * @param level  - [in] logical level on dtr line 
 * 
 * @return       - 0 on success, error code otherwise 
 */ 
typedef int32_t (*userial_dtr_set_t)(struct userial * restrict const this,
                                     const bool level);

/***
 * @brief read data from serial port
 * @param this      - [inout] serial port instance
 * @param dst       - [out] serial port read destination
 * @param dst_siz   - [in] destination size
 * 
 * @return          - read data count on success, error code otherwise 
 */ 
typedef int32_t (*userial_read_t)(struct userial * restrict const this,
                                  uint8_t * restrict const dst,
                                  const uint32_t dst_siz);

/***
 * @brief write data to serial port
 * @param this      - [inout] serial port instance
 * @param dst       - [out] serial port read destination
 * @param dst_siz   - [in] destination size
 * 
 * @return          - write data count on success, error code otherwise 
 */ 
typedef int32_t (*userial_write_t)(struct userial * restrict const this,
                                  const uint8_t * restrict const src,
                                  const uint32_t src_siz);

/***
 * @struct unified serial port instance, generic type for access the serial port
 */ 
typedef struct userial {
    /* unified serial port function table */
    userial_ctor_t ctor;            /* open serial port */
    userial_dtor_t dtor;            /* close serial port */
    userial_speed_set_t speed_set;  /* set speed for serial port */
    userial_flush_t flush;          /* flush serial port */
    userial_setup_t setup;          /* set serial port parameters */
    userial_rts_set_t rts_set;      /* set level on rts line */
    userial_dtr_set_t dtr_set;      /* set level on dtr line */
    userial_read_t read;            /* read data from serial port */
    userial_write_t write;          /* write data from serial port */
    /* unified serial port parameters */
    uint8_t name[SERIAL_PORT_PATH_MAX];
    uint32_t initiated;
    uint32_t speed;
    uint8_t databits;
    uint8_t stopbits;
    userial_parity_t parity;
} userial_t;

#endif