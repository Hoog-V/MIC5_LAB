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
#include "event_groups.h"
#include "queue.h"
#include "task.h"
#include "timers.h"

#include "rgb.h"
#include "serial.h"

/*----------------------------------------------------------------------------*/
// Local defines
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vTask(void *parameters);
static void vEventReader(void *parameters);

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/
static EventGroupHandle_t xEventGroup;

const EventBits_t xEventBits[3] =
{
    0b00000000000000000000000000000001, // Is set by vTask1
    0b00000000000000000000000000000010, // Is set by vTask2
    0b00000000000000000000000000000100, // Is set by vTask3
};

const TickType_t xDelaysMs[3] =
{
    pdMS_TO_TICKS(1000),  // Delay for vTask1
    pdMS_TO_TICKS(5000),  // Delay for vTask2
    pdMS_TO_TICKS(10000), // Delay for vTask3
};

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    rgb_init();
    xSerialPortInit(921600, 128);

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 6 - Example 01\r\n");
    vSerialPutString("By Hugo Arends\r\n\r\n");

    xTaskCreate(vTask,        "vTask1",       configMINIMAL_STACK_SIZE, (void *)1, 3, NULL );
    xTaskCreate(vTask,        "vTask2",       configMINIMAL_STACK_SIZE, (void *)2, 2, NULL );
    xTaskCreate(vTask,        "vTask3",       configMINIMAL_STACK_SIZE, (void *)3, 1, NULL );
    xTaskCreate(vEventReader, "vEventReader", configMINIMAL_STACK_SIZE, NULL,      4, NULL );

    // Create the event group
    xEventGroup = xEventGroupCreate();

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created.
    Chapter 2 provides more information on heap memory management. */
    for( ;; );
}

/*----------------------------------------------------------------------------*/

static void vTask(void *parameters)
{
    char str[48];

    // Set task specific values.
    int32_t n = (int32_t)parameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    TickType_t xDelayMs = xDelaysMs[n-1];

    sprintf(str, "[% 7ld - Task %ld     ] Created\r\n", xLastWakeTime, n);
    vSerialPutString(str);

    // As per most tasks, this task is implemented in an infinite loop.
    for( ;; )
    {
        // Wait
        vTaskDelayUntil(&xLastWakeTime, xDelayMs);

        // Print info
        sprintf(str, "[% 7ld - Task %ld     ] Set bit %ld\r\n", xLastWakeTime, n, (n-1));
        vSerialPutString(str);

        // Set the bit in the event group
        xEventGroupSetBits(xEventGroup, xEventBits[n-1]);
    }
}

/*----------------------------------------------------------------------------*/

static void vEventReader(void *parameters)
{
    char str[64];

    EventBits_t xEventGroupValue;
    const EventBits_t xBitsToWaitFor = xEventBits[0] | xEventBits[1] | xEventBits[2];

#if 1

    const BaseType_t xWaitForAllBits = pdTRUE;

#else

    const BaseType_t xWaitForAllBits = pdFALSE;

#endif



    sprintf(str, "[% 7ld - EventReader] Created\r\n", xTaskGetTickCount());
    vSerialPutString(str);

    // As per most tasks, this task is implemented in an infinite loop.
    for( ;; )
    {
        // Block to wait for event bits to become set within the event group.
        xEventGroupValue = xEventGroupWaitBits(xEventGroup,
                                               xBitsToWaitFor,
                                               pdTRUE,
                                               xWaitForAllBits,
                                               portMAX_DELAY);

        // Prepare the string
        sprintf(str, "[% 7ld - EventReader] EventBits: 000..0", xTaskGetTickCount());

        // Add the value of the last three bits
        strcat(str, ((xEventGroupValue & xEventBits[2]) != 0) ? "1" : "0");
        strcat(str, ((xEventGroupValue & xEventBits[1]) != 0) ? "1" : "0");
        strcat(str, ((xEventGroupValue & xEventBits[0]) != 0) ? "1" : "0");

        // Finalize string and print it
        strcat(str, "\r\n");
        vSerialPutString(str);
    }
}
