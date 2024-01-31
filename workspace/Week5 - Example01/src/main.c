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
// Local defines
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vTask(void *parameters);

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

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 5 - Example 01\r\n");
    vSerialPutString("By Hugo Arends\r\n\r\n");

    vSerialPutString("Example demonstrating INCORRECT Resource Management\r\n\r\n");

    // Two tasks with the same priority
    xTaskCreate( vTask, "Task 1", configMINIMAL_STACK_SIZE, "Task 1", 1, NULL );
    xTaskCreate( vTask, "Task 2", configMINIMAL_STACK_SIZE, "Task 2", 1, NULL );

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created.
    Chapter 2 provides more information on heap memory management. */
    for( ;; );
}

/*----------------------------------------------------------------------------*/

void vTask( void *pvParameters )
{
    // The string to print out is passed in via the parameter. Cast this to a
    // character pointer.
    char *pcTaskName = (char *)pvParameters;

    // As per most tasks, this task is implemented in an infinite loop.
    for( ;; )
    {
        // Print out the name of this task
        vSerialPutString(pcTaskName);

        // Print out the number of this task several times
        for(uint32_t i = 0; i<15; i++)
        {
            // Write the task number to the xCharsForTx
            // queue
            xSerialPutChar(pcTaskName[5], 0);
        }

        // Terminate the string
        vSerialPutString("\r\n");
    }
}
