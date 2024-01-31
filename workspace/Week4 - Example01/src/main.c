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
#include "semphr.h"

#include "rgb.h"
#include "serial.h"
#include "timer.h"

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vPeriodicTask(void *pvParameters);
static void vHandlerTask(void *pvParameters);

static void delay_us(uint32_t d);

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

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 4 - Example 01\r\n");
    vSerialPutString("By Hugo Arends\r\n\r\n");

    /* Before a semaphore is used it must be explicitly created. In this example a
    counting semaphore is created. The semaphore is created to have a maximum count
    value of 3, and an initial count value of 0. */
    xCountingSemaphore = xSemaphoreCreateCounting(3, 0);

    /* Check the semaphore was created successfully. */
    if( xCountingSemaphore == NULL )
    {
        /* Error, unable to create the semaphore */
        rgb_red_on(true);
        while(1)
        {;}
    }

    // Semaphore are implemented by using queues that store not data items.
    // We can therefor visualize a sempahore in the kernel aware debugger by
    // registering it as we would do with a queue.
    vQueueAddToRegistry(xCountingSemaphore, "xCountingSemaphore");

    // Create the tasks
    xTaskCreate(vPeriodicTask, "Periodic", configMINIMAL_STACK_SIZE, NULL, 1, NULL );
    xTaskCreate(vHandlerTask,  "Handler",  configMINIMAL_STACK_SIZE, NULL, 3, NULL );

    // Initialize the timer that generates interrupts. An interrupt is generated
    // every 1 ms. Every two seconds, the semaphore will be given - see the ISR
    // TPM1_IRQHandler in timer.c
    tim_init();

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient FreeRTOS heap memory available for the idle task to be
    created. Chapter 2 provides more information on heap memory management. */
    for( ;; );
}

/*----------------------------------------------------------------------------*/

// Software delay of approximately 1 us, depending on CPU clock frequency
// and optimization level
// - CPU clock: 48 MHz
// - Optimization level: -O3
// - Optimize for time: disabled
static void delay_us(uint32_t d)
{

#if (CLOCK_SETUP != 1)
  #warning This delay function does not work as designed
#endif

    volatile uint32_t t;

    for(t=4*d; t>0; t--)
    {
        __asm("nop");
        __asm("nop");
    }
}

/*----------------------------------------------------------------------------*/

static void vPeriodicTask( void *pvParameters )
{
    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        // Do not use vTaskDelay(), because that would block this task.
        // Use a for-loop to create a delay instead, which means this task
        // can always be in Running state, except when it is pre-empted by a
        // higher priority task or ISR.

        rgb_green_on(true);
        delay_us(500000);

        rgb_green_on(false);
        delay_us(500000);
    }
}

/*----------------------------------------------------------------------------*/

static void vHandlerTask( void *pvParameters )
{
    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        /* Use the semaphore to wait for the event. The semaphore was created
        before the scheduler was started, so before this task ran for the first
        time. The task blocks indefinitely, meaning this function call will only
        return once the semaphore has been successfully obtained - so there is
        no need to check the value returned by xSemaphoreTake(). */
        xSemaphoreTake( xCountingSemaphore, portMAX_DELAY );

        /* To get here the event must have occurred. Process the event (in this
        Case, just print out a message). */
        vSerialPutString( "[Handler task] Processing event\r\n" );
    }
}
