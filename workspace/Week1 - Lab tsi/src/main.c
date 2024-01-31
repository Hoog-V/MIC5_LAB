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

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vTask1(void *parameters);
static void vTask2(void *parameters);

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    rgb_init();
    xSerialPortInit(921600, 128);

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 1 - Lab exercise\r\n\r\n");

    xTaskCreate( vTask1, "Task 1", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    xTaskCreate( vTask2, "Task 2", configMINIMAL_STACK_SIZE, NULL, 4, NULL );

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created.
    Chapter 2 provides more information on heap memory management. */
    for( ;; );
}

/*----------------------------------------------------------------------------*/

void vTask1( void *pvParameters )
{
	const TickType_t xDelay100ms = pdMS_TO_TICKS( 100 );
	const TickType_t xDelay900ms = pdMS_TO_TICKS( 900 );

    /* As per most tasks, this task is implemented in an infinite loop. */
    for( ;; )
    {
    	rgb_green_on(true);
		vTaskDelay( xDelay100ms );
    	rgb_green_on(false);
		vTaskDelay( xDelay900ms );
    }
}

/*----------------------------------------------------------------------------*/

void vTask2( void *pvParameters )
{
    // Enable PTB clock
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    // Set pin to FTM
    // PTB16: TSI0_CH9, Mux Alt 0 (default)
    // PTB17: TSI0_CH10, Mux Alt 0 (default)
    PORTB->PCR[9] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[9] |= PORT_PCR_MUX(0);
    PORTB->PCR[10] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[10] |= PORT_PCR_MUX(0);

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

    /* As per most tasks, this task is implemented in an infinite loop. */

    static uint32_t channel09_result;
    static uint32_t channel10_result;

	TickType_t xLastWakeTime = xTaskGetTickCount();

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
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 1 ) );

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
        // Combine the scan results
        // ------------------------------------------------------------------------
        // The variables channel09_result and channel10_result now hold the
        // accumulated scan counter values (32 scans as set by TSI_GENCS_NSCN).
        // The following threshold values are determined by using the debugger.
        //
        //              channel09_result  channel10_result
        // No touch           0x0000025B        0x00000272
        // Left touch         0x00000600        0x000003A0
        // Center touch       0x000004A0        0x000004B0
        // Right touch        0x00000300        0x00000600
        //
        // Set touch according to the scan results of the two channels combined
        if(channel09_result > 0x00000380 && channel10_result < 0x00000380)
        {
            rgb_blue_on(true);
            rgb_red_on(false);
        }
        else if(channel09_result > 0x00000380 && channel10_result > 0x00000380)
        {
            rgb_blue_on(false);
            rgb_red_on(false);
        }
        else if(channel09_result < 0x00000380 && channel10_result > 0x00000380)
        {
            rgb_blue_on(false);
            rgb_red_on(true);
        }
        else
        {
            rgb_blue_on(false);
            rgb_red_on(false);
        }

        // Wait before sampling the next time
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 20 ) );
    }
}

/*----------------------------------------------------------------------------*/
