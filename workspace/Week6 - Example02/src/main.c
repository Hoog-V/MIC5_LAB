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

/*----------------------------------------------------------------------------*/
// Local defines
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vTask(void *parameters);

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

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    rgb_init();
    xSerialPortInit(921600, 128);

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 6 - Example 02\r\n");
    vSerialPutString("By Hugo Arends\r\n\r\n");

    xTaskCreate(vTask, "vTask1", configMINIMAL_STACK_SIZE, (void *)1, 1, NULL );
    xTaskCreate(vTask, "vTask2", configMINIMAL_STACK_SIZE, (void *)2, 1, NULL );
    xTaskCreate(vTask, "vTask3", configMINIMAL_STACK_SIZE, (void *)3, 1, NULL );

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

    int32_t n = (int32_t)parameters;

    TickType_t xDelayTime;
    const TickType_t xMinDelay = pdMS_TO_TICKS(200UL);
    const TickType_t xMaxDelay = pdMS_TO_TICKS(5000UL);
    const EventBits_t uxAllSyncBits = xEventBits[0] | xEventBits[1] | xEventBits[2];

    sprintf(str, "[% 7ld - Task %ld     ] Created\r\n", xTaskGetTickCount(), n);
    vSerialPutString(str);

    // As per most tasks, this task is implemented in an infinite loop.
    for( ;; )
    {
        // Simulate this task taking some time to perform an action by delaying for a
        // pseudo random time. This prevents all three instances of this task reaching
        // the synchronization point at the same time, and so allows the example’s
        // behavior to be observed more easily.
        xDelayTime = (rand() % xMaxDelay) + xMinDelay;
        vTaskDelay(xDelayTime);

        // Print out a message to show this task has reached its synchronization
        // point.
        sprintf(str, "[% 7ld - Task %ld     ] Reached sync point\r\n", xTaskGetTickCount(), n);
        vSerialPutString(str);

        // Wait for all the tasks to have reached their respective synchronization
        // points.
        xEventGroupSync(xEventGroup, xEventBits[n-1], uxAllSyncBits, portMAX_DELAY);

        // Print out a message to show this task has passed its synchronization
        // point. As an indefinite delay was used the following line will only be
        // executed after all the tasks reached their respective synchronization
        // points.
        sprintf(str, "[% 7ld - Task %ld     ] Exited sync point\r\n", xTaskGetTickCount(), n);
        vSerialPutString(str);
    }
}
