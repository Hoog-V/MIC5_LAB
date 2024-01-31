/*! ***************************************************************************
 *
 * \brief     I2C low level peripheral driver
 * \file      i2c0.c
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
#include <MKL25Z4.h>
#include "i2c0.h"

static void delay_us(uint32_t d)
{

#if (CLOCK_SETUP != 1)
#warning This delay function does not work as designed
#endif

    volatile uint32_t t;

    for(t=4*d; t>0; t--)
    {
        __asm("nop");
        __asm("nop");
    }
}

// Init i2c0
void i2c0_init(void)
{
    // Clock I2C peripheral and port
    SIM->SCGC4 |= SIM_SCGC4_I2C0_MASK;
	SIM->SCGC5 |= (SIM_SCGC5_PORTE_MASK);

	// Set pins to I2C function
	PORTE->PCR[24] |= PORT_PCR_MUX(5);
	PORTE->PCR[25] |= PORT_PCR_MUX(5);

    // MUL[1:0] : The MULT bits define the multiplier factor mul.
    // ICR[5:0] : Prescales the bus clock for bit rate selection.
    //
    // I2C baud rate = bus speed (Hz)/(mul � SCL divider)
    //        375000 =     24MHz     /( 1  *    64)
 	I2C0->F = (I2C_F_ICR(0x12) | I2C_F_MULT(0));

    // Enable i2c and set to master mode
    I2C0->C1 |= (I2C_C1_IICEN_MASK);
}

// Start sequence
void i2c0_start()
{
    // Make sure bus free time is 1.3 us (t_BUF).
    delay_us(2);

    // Set to transmit mode
    I2C0->C1 |= I2C_C1_TX_MASK;

    // Generate start condition
    I2C0->C1 |= I2C_C1_MST_MASK;
}

// Send device and register addresses
bool i2c0_read_setup(uint8_t address, uint8_t reg)
{
    // Send the address
    I2C0->D = address;

    // Wait for acknowledge
    // Return anyway if it takes too long
    uint32_t timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

    // Send register
    I2C0->D = reg;

    // Wait for acknowledge
    // If timeout occurs, try to reinitialise i2c and return from this function
    timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

    // Repeated start
    I2C0->C1 |= I2C_C1_RSTA_MASK;

    // Send device address (read)
    I2C0->D = (address | 0x01);

    // Wait for acknowledge
    // If timeout occurs, try to reinitialise i2c and return from this function
    timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

    // Receive mode
    I2C0->C1 &= ~I2C_C1_TX_MASK;

    return true;
}

// Read a repeated byte and ack/nack as appropriate
bool i2c0_repeated_read(bool last, uint8_t *data)
{
	if(last)
    {
        // NACK after read
        I2C0->C1 |= I2C_C1_TXAK_MASK;
	}
    else
    {
        // ACK after read
        I2C0->C1 &= ~I2C_C1_TXAK_MASK;
	}

    // Dummy read
    *data = I2C0->D;

    // Wait for acknowledge
    // If timeout occurs, try to reinitialise i2c and return from this function
    uint32_t timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

	if(last)
    {
        // Send stop
        I2C0->C1 &= ~I2C_C1_MST_MASK;
	}

    // Read data
    *data = I2C0->D;

	return true;
}

// Functions for reading and writing a single byte

bool i2c0_read_byte(uint8_t address, uint8_t reg, uint8_t *data)
{
    // Make sure bus free time is 1.3 us (t_BUF).
    delay_us(2);

    // Set to transmit mode
    I2C0->C1 |= I2C_C1_TX_MASK;

    // Generate start condition
    I2C0->C1 |= I2C_C1_MST_MASK;

    // Send the address
    I2C0->D = address;

    // Wait for acknowledge
    // Return anyway if it takes too long
    uint32_t timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

    // Send register
    I2C0->D = reg;

    // Wait for acknowledge
    // If timeout occurs, try to reinitialise i2c and return from this function
    timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

    // Repeated start
    I2C0->C1 |= I2C_C1_RSTA_MASK;

    // Send device address (read)
    I2C0->D = (address | 0x01);

    // Wait for acknowledge
    // If timeout occurs, try to reinitialise i2c and return from this function
    timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

    // Receive mode
    I2C0->C1 &= ~I2C_C1_TX_MASK;

    // NACK after read
    I2C0->C1 |= I2C_C1_TXAK_MASK;

    // Dummy read
    *data = I2C0->D;

    // Wait for acknowledge
    // If timeout occurs, try to reinitialise i2c and return from this function
    timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

    // Send stop
    I2C0->C1 &= ~I2C_C1_MST_MASK;

    // Read LSB
    *data = I2C0->D;

	return true;
}

bool i2c0_write_byte(uint8_t address, uint8_t reg, uint8_t data)
{
    // Make sure bus free time is 1.3 us (t_BUF).
    delay_us(2);

    // Set to transmit mode
    I2C0->C1 |= I2C_C1_TX_MASK;

    // Generate start condition
    I2C0->C1 |= I2C_C1_MST_MASK;

    // Send the address
    I2C0->D = address;

    // Wait for acknowledge
    // Return anyway if it takes too long
    uint32_t timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

    // Send register
    I2C0->D = reg;

    // Wait for acknowledge
    // If timeout occurs, try to reinitialise i2c and return from this function
    timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

    // Send lsb
    I2C0->D = data;

    // Wait for acknowledge
    // If timeout occurs, try to reinitialise i2c and return from this function
    timeout = I2C_TIMEOUT;
    while((I2C0->S & I2C_S_IICIF_MASK)==0)
    {
        if(--timeout == 0)
        {
            return false;
        }
    }

    // Clear the flag
    I2C0->S |= I2C_S_IICIF_MASK;

    // Send stop
    I2C0->C1 &= ~I2C_C1_MST_MASK;

    return true;
}
