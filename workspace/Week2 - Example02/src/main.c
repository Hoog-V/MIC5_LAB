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

/*----------------------------------------------------------------------------*/
// Local defines
/*----------------------------------------------------------------------------*/
#define mainN_TASKS 3
//#define mainN_TASKS 9

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vTaskFunction(void *parameters);

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/
StaticTask_t xTaskBuffers[ mainN_TASKS ];

StackType_t xStack[ mainN_TASKS ][ configMINIMAL_STACK_SIZE ];

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    rgb_init();

	xTaskCreateStatic( vTaskFunction, "Task 1", configMINIMAL_STACK_SIZE, (void *)"Task 1", 1, xStack[0], &xTaskBuffers[0] );
	xTaskCreateStatic( vTaskFunction, "Task 2", configMINIMAL_STACK_SIZE, (void *)"Task 2", 2, xStack[1], &xTaskBuffers[1] );
	xTaskCreateStatic( vTaskFunction, "Task 3", configMINIMAL_STACK_SIZE, (void *)"Task 3", 3, xStack[2], &xTaskBuffers[2] );

	// How many more tasks can be added until memory allocation fails?

#if (mainN_TASKS == 9)

	xTaskCreateStatic( vTaskFunction, "Task 4", configMINIMAL_STACK_SIZE, (void *)"Task 4", 4, xStack[3], &xTaskBuffers[3] );
	xTaskCreateStatic( vTaskFunction, "Task 5", configMINIMAL_STACK_SIZE, (void *)"Task 5", 5, xStack[4], &xTaskBuffers[4] );
	xTaskCreateStatic( vTaskFunction, "Task 6", configMINIMAL_STACK_SIZE, (void *)"Task 6", 6, xStack[5], &xTaskBuffers[5] );
	xTaskCreateStatic( vTaskFunction, "Task 7", configMINIMAL_STACK_SIZE, (void *)"Task 7", 7, xStack[6], &xTaskBuffers[6] );
	xTaskCreateStatic( vTaskFunction, "Task 8", configMINIMAL_STACK_SIZE, (void *)"Task 8", 8, xStack[7], &xTaskBuffers[7] );
	xTaskCreateStatic( vTaskFunction, "Task 9", configMINIMAL_STACK_SIZE, (void *)"Task 9", 9, xStack[8], &xTaskBuffers[8] );

#endif

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

/* configUSE_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
	/* If the buffers to be provided to the Idle task are declared inside this
	function then they must be declared static - otherwise they will be allocated on
	the stack and so not exists after this function exits. */
	static StaticTask_t xIdleTaskTCB;
	static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

	/* Pass out a pointer to the StaticTask_t structure in which the Idle task's
	state will be stored. */
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppxIdleTaskStackBuffer = uxIdleTaskStack;

	/* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/*----------------------------------------------------------------------------*/

void vTaskFunction( void *pvParameters )
{
	const TickType_t xDelay1000ms = pdMS_TO_TICKS( 1000 );

	/* As per most tasks, this task is implemented in an infinite loop. */
	for( ;; )
	{
		/* Delay for a period of 1000 milliseconds. */
		vTaskDelay( xDelay1000ms );
	}
}
