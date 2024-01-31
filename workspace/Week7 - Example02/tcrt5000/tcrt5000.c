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

    // - ADTRG = 1   : Hardware trigger selected
    // - ACFE  = 0   : Compare function disabled
    // - DMAEN = 0   : DMA is disabled
    // - REFSEL = 00 : Default voltage reference pin pair
    ADC0->SC2 = ADC_SC2_ADTRG(1);

    // - AIEN = 1     : Conversion complete interrupt is enabled
    // - DIFF = 0     : Single-ended conversions and input channels are
    //                  selected
    // - ADCH = 01000 : Channel 8
    ADC0->SC1[0] = ADC_SC1_AIEN(1) | ADC_SC1_ADCH(8);

    // ------------------------------------------------------------------------

    // Clock to TIM1 on
    SIM->SCGC6 |= SIM_SCGC6_TPM1(1);

    // Divide by 128 Prescale Factor
    TPM1->SC |= TPM_SC_PS(0b111);

    // (48 MHz / 128 ) / 20 Hz = 18750
    TPM1->MOD = (18750-1);

    // Counter increments on every LPTPM counter clock
    TPM1->SC |= TPM_SC_CMOD(1);

    // ------------------------------------------------------------------------

    // ADC0 trigger source select
    // - ADC0ALTTRGEN = 1  : Alternate trigger selected for ADC0
    // - ADC0PRETRGSEL = 0 : Pre-trigger A
    // - ADC0TRGSEL = 1001 : TPM1 overflow
    SIM->SOPT7 |= SIM_SOPT7_ADC0ALTTRGEN(1) | SIM_SOPT7_ADC0TRGSEL(9);

    // ------------------------------------------------------------------------

    // Enable the interrupt in the NVIC
    NVIC_SetPriority(ADC0_IRQn, 128);
    NVIC_ClearPendingIRQ(ADC0_IRQn);
    NVIC_EnableIRQ(ADC0_IRQn);
}

void ADC0_IRQHandler(void)
{
    // Clear pending interrupt
    NVIC_ClearPendingIRQ(ADC0_IRQn);

    // Clear the flag
    TPM1->STATUS = TPM_STATUS_TOF(1);

    static volatile bool ir_led_is_on = false;
    static volatile uint32_t on_brightness;
    static volatile uint32_t off_brightness;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t xResult;

    if(ir_led_is_on)
    {
        // Get conversion and complement the result
        on_brightness = 0xFFFF - ADC0->R[0];

        // IR LED off
        PTA->PSOR = (1<<16);
        ir_led_is_on = false;

        // Send a notification, and the ADC conversion result, directly to
        // vADCTask()
        xResult = xTaskNotifyFromISR(xADCTaskHandle,
                                     on_brightness - off_brightness,
                                     eSetValueWithoutOverwrite,
                                     &xHigherPriorityTaskWoken );

        // If the call to xTaskNotifyFromISR() doesn't return pdPASS then the
        // vADCTask is not keeping up with the rate at which ADC values are
        // being generated
        configASSERT(xResult == pdPASS);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    else
    {
        // Get conversion and complement the result
        off_brightness = 0xFFFF - ADC0->R[0];

        // IR LED on
        PTA->PCOR = (1<<16);
        ir_led_is_on = true;
    }
}
