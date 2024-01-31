/*! ***************************************************************************
 *
 * \brief     Low level driver for periodically generating interrupts
 * \file      timer.c
 * \author    Hugo Arends
 * \date      February 2022
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
#include "timer.h"

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/

void tim_init(void)
{
    // Clock to TIM1 on
    SIM->SCGC6 |= SIM_SCGC6_TPM1(1);

    // 48 MHz / 48000 = 1kHz
//    TPM1->MOD = (48000-1);
    TPM1->MOD = 0xFFFF;

    // Divide by 1 Prescale Factor
//    TPM1->SC &= ~TPM_SC_PS_MASK;
    TPM1->SC |= TPM_SC_PS(0b111);



//    // Timer overflow interrupt enable
//    TPM1->SC |= TPM_SC_TOIE(1);

    // Counter increments on every LPTPM counter clock
    TPM1->SC |= TPM_SC_CMOD(1);

//    // Enable Interrupts
//    NVIC_SetPriority(TPM1_IRQn, 0);
//    NVIC_ClearPendingIRQ(TPM1_IRQn);
//    NVIC_EnableIRQ(TPM1_IRQn);
}

void TPM1_IRQHandler(void)
{

    NVIC_ClearPendingIRQ(TPM1_IRQn);

    if(TPM1->STATUS & TPM_STATUS_TOF(1))
    {
        // Reset the interrupt flag
        TPM1->STATUS = TPM_STATUS_TOF(1);
    }
}
