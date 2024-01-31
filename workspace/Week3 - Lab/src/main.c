/*! ***************************************************************************
 *
 * \brief     FreeRTOS port for FRDM-KL25Z
 * \file      main.c
 * \author    Hugo Arends
 * \date      October 2021
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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

#include "rgb.h"
#include "serial.h"
#include "switches.h"

/*----------------------------------------------------------------------------*/
// Local type definitions
/*----------------------------------------------------------------------------*/
typedef enum
{
    UP,
    DOWN,
}command_t;

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vSWSamplerTask(void *parameters);
static void vTSISamplerTask(void *parameters);
static void vCmdHandlerTask(void *parameters);

static void prvLedsOffCallback(TimerHandle_t xTimer);

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    rgb_init();
    sw_init();
    xSerialPortInit(921600, 128);

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS Week 3 - Lab\r\n\r\n");

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created.
    Chapter 2 provides more information on heap memory management. */
    for( ;; );
}

/*----------------------------------------------------------------------------*/

/*
 * TODO: Carefully study this task. When a touch is detected, send the
 * appropriate command to the command queue.
 */
static void vTSISamplerTask(void *parameters)
{
    vSerialPutString("[TSISampler] Created\r\n");

    // Enable PTB clock
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    // Set pin to FTM
    // PTB16: TSI0_CH9, Mux Alt 0 (default)
    // PTB17: TSI0_CH10, Mux Alt 0 (default)
    PORTB->PCR[16] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[16] |= PORT_PCR_MUX(0);
    PORTB->PCR[17] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[17] |= PORT_PCR_MUX(0);

    // Enable TSI Clock
    SIM->SCGC5 |= SIM_SCGC5_TSI_MASK;

    // Setup general control and status register
    // - OUTRGF : 1 - Clear Out-of-range flag
    // - ESOR   : 0 - Out-of-range interrupt is allowed (default)
    // - MODE   : 0000 - Set TSI in capacitive sensing(non-noise detection) mode
    //                   (default)
    // - REFCHRG: 000 - 500 nA reference oscillator charge and discharge current
    //                  value (default)
    // - DVOLT  : 00 - DV = 1.03 V; V P = 1.33 V; V m = 0.30 V oscillator's
    //                 voltage rails (default)
    // - EXTCHRG: 000 - 500 nA electrode oscillator charge and discharge current
    //                  (default)
    // - PS     : 0 - Electrode Oscillator Frequency divided by 1 prescaler
    //                (default)
    // - NSCN   : 11111 - 32 scans for each electrode
    // - TSIEN  : 1 - TSI module enabled
    // - TSIIEN : 0 - TSI interrupt is disabled (default)
    // - STPE   : 1 - Allow TSI to continue running in all low power modes
    // - STM    : 0 - Software trigger scan (default)
    // - SCNIP  : n/a
    // - EOSF   : 1 - Clear scan complete flag
    // - CURSW  : 0 - The current source pair are not swapped (default)
    TSI0->GENCS =
        TSI_GENCS_OUTRGF(1) |
        TSI_GENCS_NSCN(31) |
        TSI_GENCS_TSIEN(1) |
        TSI_GENCS_STPE(1) |
        TSI_GENCS_EOSF(1);

    static uint32_t channel09_result;
    static uint32_t channel10_result;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    /* As per most tasks, this task is implemented in an infinite loop. */
    for( ;; )
    {

        // ------------------------------------------------------------------------
        // Scan channel 9
        // ------------------------------------------------------------------------

        // Select channel 9
        TSI0->DATA = TSI_DATA_TSICH(9);

        // Start a scan
        TSI0->DATA |= TSI_DATA_SWTS_MASK;

        // Wait for scan complete
        while(!(TSI0->GENCS & TSI_GENCS_EOSF_MASK))
        {;}

        // Read TSI Conversion Counter Value, masking all other bits
        channel09_result = TSI0->DATA & TSI_DATA_TSICNT_MASK;

        // Clear scan complete flag
        TSI0->GENCS |= TSI_GENCS_EOSF_MASK;

        // Wait some time before sampling the next channel
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(1) );

        // ------------------------------------------------------------------------
        // Scan channel 10
        // ------------------------------------------------------------------------

        // Select channel 10
        TSI0->DATA = TSI_DATA_TSICH(10);

        // Start a scan
        TSI0->DATA |= TSI_DATA_SWTS_MASK;

        // Wait for scan complete
        while(!(TSI0->GENCS & TSI_GENCS_EOSF_MASK))
        {;}

        // Read TSI Conversion Counter Value, masking all other bits
        channel10_result = TSI0->DATA & TSI_DATA_TSICNT_MASK;

        // Clear scan complete flag
        TSI0->GENCS |= TSI_GENCS_EOSF_MASK;

        // ------------------------------------------------------------------------
        // Calculate the scan results
        // ------------------------------------------------------------------------
        // The variables channel09_result and channel10_result now hold the
        // accumulated scan counter values (32 scans as set by TSI_GENCS_NSCN).
        // The following threshold value is determined by using the debugger.
        const uint32_t threshold = 0x00000380;

        // Send command according the scan results
        if(channel09_result > threshold)
        {
            vSerialPutString("[TSISampler] Channel 09 touched\r\n");
        }

        if(channel10_result > threshold)
        {
            vSerialPutString("[TSISampler] Channel 10 touched\r\n");
        }

        // Wait before sampling the next time
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(199) );
    }
}
