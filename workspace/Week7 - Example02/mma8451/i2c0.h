/*! ***************************************************************************
 *
 * \brief     I2C low level peripheral driver
 * \file      i2c0.h
 * \author    Hugo Arends
 * \date      April 2021
 *
 * \remark    Hardware connection
 * <pre>                                MMA8451Q accelerometer           </pre>
 * <pre>                           Vdd +-------------+                   </pre>
 * <pre>      FRDM-KL25Z            |  |             |                   </pre>
 * <pre>      -------------+        +--+Vcc          |                   </pre>
 * <pre>                   |   GND|----+GND          |                   </pre>
 * <pre>     I2C0_SCL/PTE24+-----------+SCL          |                   </pre>
 * <pre>     I2C0_SDA/PTE25+-----------+SDA          |                   </pre>
 * <pre>                   |           |             |                   </pre>
 * <pre>              PTA14+-----------+INT1         |                   </pre>
 * <pre>              PTA15+-----------+INT2         |                   </pre>
 * <pre>                   |           |             |                   </pre>
 * <pre>      -------------+           +-------------+                   </pre>
 *
 * \copyright 2021 HAN University of Applied Sciences. All Rights Reserved.
 *            \n\n
 *            Permission is hereby granted, free of charge, to any person
 *            obtaining a copy of this software and associated documentation
 *            files (the "Software"), to deal in the Software without
 *            restriction, including without limitation the rights to use,
 *            copy, modify, merge, publish, distribute, sublicense, and/or sell
 *            copies of the Software, and to permit persons to whom the
 *            Software is furnished to do so, subject to the following
 *            conditions:
 *            \n\n
 *            The above copyright notice and this permission notice shall be
 *            included in all copies or substantial portions of the Software.
 *            \n\n
 *            THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *            EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *            OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *            NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *            HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *            WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *            FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *            OTHER DEALINGS IN THE SOFTWARE.
 *
 *****************************************************************************/
#ifndef I2C0_H
#define I2C0_H

#include <stdint.h>
#include <stdbool.h>

/*!
 * \brief Definition for the I2C timeout
 *
 * This timeout value is used in loops to wait for a bit to set/reset. If the
 * bit doesn't get set, the function returns.
 */
#define I2C_TIMEOUT (10000)

void i2c0_init(void);

void i2c0_start(void);
bool i2c0_read_setup(uint8_t address, uint8_t reg);
bool i2c0_repeated_read(bool last, uint8_t *data);
	
bool i2c0_read_byte(uint8_t address, uint8_t reg, uint8_t *data);
bool i2c0_write_byte(uint8_t address, uint8_t reg, uint8_t data);

#endif // I2C0_H
