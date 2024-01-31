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
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

#include "rgb.h"
#include "serial.h"
#include "tcrt5000.h"

/*----------------------------------------------------------------------------*/
// Local defines
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vADCTask(void *parameters);

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    rgb_init();
    tcrt5000_init();
    xSerialPortInit(921600, 128);

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 6 - Example 03\r\n");
    vSerialPutString("By Hugo Arends\r\n\r\n");

    xTaskCreate(vADCTask, "vADCTask", configMINIMAL_STACK_SIZE, NULL, 1, &xADCTaskHandle);

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created.
    Chapter 2 provides more information on heap memory management. */
    for( ;; );
}

/*----------------------------------------------------------------------------*/

static void vADCTask(void *parameters)
{
    char str[128];

    // Enable the second line to see what happens if this task is not notified
    // within the expected timeout time
    const TickType_t xADCConversionTimeout = pdMS_TO_TICKS(110);
//    const TickType_t xADCConversionTimeout = pdMS_TO_TICKS(60);

    uint32_t ulADCResult;
    BaseType_t xResult;

    sprintf(str, "[vADCTask] Created\r\n");
    vSerialPutString(str);

    // As per most tasks, this task is implemented in an infinite loop.
    for( ;; )
    {
        // Enable this line to simulate that this task does not keep up with
        // the rate at conversion results are notified.
        // TIP. You need the debugger to see where code execution stopped.
//        vTaskDelay(pdMS_TO_TICKS(200));

        // Wait for the next ADC conversion result
        xResult = xTaskNotifyWait(0, // The new ADC value will overwrite the
                                     // old value, so there is no need to clear
                                     // any bits before waiting for the new
                                     // notification value
                                  0, // Future ADC values will overwrite the
                                     // existing value, so there is no need to
                                     // clear any bits before exiting
                                     // xTaskNotifyWait()
                                  &ulADCResult,
                                  xADCConversionTimeout);

        // Check the notification result
        if(xResult == pdPASS)
        {
            // Notification received
            sprintf(str, "[vADCTask] % 5ld\r\n", ulADCResult);
            vSerialPutString(str);

            // Process the ADC result
            rgb_green_on(ulADCResult < 2000);
            rgb_red_on(ulADCResult >= 2000);
        }
        else
        {
            // No notification received (timed out)
            sprintf(str, "[vADCTask] No ADC result within expected time\r\n");
            vSerialPutString(str);
        }
    }
}
