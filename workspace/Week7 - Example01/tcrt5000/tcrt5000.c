/*! ***************************************************************************
 *
 * \brief     Low level driver for the TCRT5000
 * \file      tcrt5000.c
 * \author    Hugo Arends
 * \date      March 2022
 *
 * \copyright 2022 HAN University of Applied Sciences. All Rights Reserved.
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
#include "tcrt5000.h"
#include "stdbool.h"

TaskHandle_t xADCTaskHandle;

/*!
 * \brief Initializes the TCRT5000 on the shield
 *
 * This functions initializes the TCRT5000 on the shield.
 * - PTA16 is configured as an output pin
 * - PTB0 is configured as an analog input (ADC channel 8)
 * - TPM1 is configured to trigger an ADC conversion every 50 ms
 */
void tcrt5000_init(void)
{
    // ------------------------------------------------------------------------

    // Enable clock to PORTs
    SIM->SCGC5 |= SIM_SCGC5_PORTA(1) | SIM_SCGC5_PORTB(1);

    // The IR LED is connected to PTA16. Configure the pin as a GPIO output
    // pin.
    PORTA->PCR[16] &= ~0x7FF;
    PORTA->PCR[16] |= PORT_PCR_MUX(1) | PORT_PCR_PE(1);
    PTA->PDDR |= (1<<16);

    // IR LED off
    PTA->PSOR = (1<<16);

    // The output of the transistor is connected to PTB0. Configure the pin as
    // ADC input pin (channel 8).
    PORTB->PCR[0] &= ~0x7FF;

    // ------------------------------------------------------------------------

    // Enable clock to ADC0
    SIM->SCGC6 |= SIM_SCGC6_ADC0(1);

    // Configure ADC
    // - ADLPC = 1        : Low-power configuration. The power is reduced at
    //                      the expense of maximum clock speed.
    // - ADIV[1:0] = 00   : The divide ratio is 1 and the clock rate is input
    //                      clock.
    // - ADLSMP = 1       : Long sample time.
    // - MODE[1:0] = 11   : Single-ended 16-bit conversion
    // - ADICLK[1:0] = 01 : (Bus clock)/2
    ADC0->CFG1 = 0x9D;

    // - ADTRG = 0   : Software trigger selected
    // - ACFE  = 0   : Compare function disabled
    // - DMAEN = 0   : DMA is disabled
    // - REFSEL = 00 : Default voltage reference pin pair
    ADC0->SC2 = 0;
}
