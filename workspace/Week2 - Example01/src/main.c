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
static void vTaskFunction(void *parameters);

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

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 2 - Example 01\r\n");
    vSerialPutString("By Hugo Arends\r\n\r\n");

	xTaskCreate( vTaskFunction, "Task 1", configMINIMAL_STACK_SIZE, (void *)"Task 1", 1, NULL );
	xTaskCreate( vTaskFunction, "Task 2", configMINIMAL_STACK_SIZE, (void *)"Task 2", 2, NULL );
	xTaskCreate( vTaskFunction, "Task 3", configMINIMAL_STACK_SIZE, (void *)"Task 3", 3, NULL );

	// How many more tasks can be added until memory allocation fails?

//	xTaskCreate( vTaskFunction, "Task 4", configMINIMAL_STACK_SIZE, (void *)"Task 4", 4, NULL );
//	xTaskCreate( vTaskFunction, "Task 5", configMINIMAL_STACK_SIZE, (void *)"Task 5", 5, NULL );
//	xTaskCreate( vTaskFunction, "Task 6", configMINIMAL_STACK_SIZE, (void *)"Task 6", 6, NULL );

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created.
    Chapter 2 provides more information on heap memory management. */

	/* If vTaskScheduler() returned, no data will be transmitted via the serial
	interface. The reason for that is that the serial interface requires
	kernel objects as well, but the kernel will never be running. */

    for( ;; );
}

/*----------------------------------------------------------------------------*/


void vApplicationMallocFailedHook( void )
{
	// Visualize an erroneous situation by using a red LED
	// It is not possible to transmit a message through the serial interface,
	// because the scheduler will not run.
	rgb_on(true, false, false);
}

/*----------------------------------------------------------------------------*/

void vTaskFunction( void *pvParameters )
{
	char *pcTaskName;
	const TickType_t xDelay1000ms = pdMS_TO_TICKS( 1000 );

	/* The string to print out is passed in via the parameter. Cast this to a
	character pointer. */
	pcTaskName = ( char * ) pvParameters;

	/* As per most tasks, this task is implemented in an infinite loop. */
	for( ;; )
	{
		size_t freeHeapSizeBefore = xPortGetFreeHeapSize();

		char *str = (char *)pvPortMalloc(64);

		size_t freeHeapSizeAfter = xPortGetFreeHeapSize();

		// Print the changed heap size. Notice that allocating memory
		// requires a little over the requested 64 bytes!

		sprintf(str, "%s | Free heap before %d and after %d in bytes\r\n",
			pcTaskName, freeHeapSizeBefore, freeHeapSizeAfter);
		vSerialPutString(str);

		vPortFree(str);

		/* Delay for a period of 1000 milliseconds. */
		vTaskDelay( xDelay1000ms );
	}
}
