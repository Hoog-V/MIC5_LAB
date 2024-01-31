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
static void vKeyHitTask(void *pvParameters);
static void prvBacklightTimerCallback(TimerHandle_t xTimer);

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/
TimerHandle_t xBacklightTimer;
BaseType_t xSimulatedBacklightOn = pdFALSE;
const TickType_t xBacklightOffDelay = pdMS_TO_TICKS(3000);

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    rgb_init();
    xSerialPortInit(921600, 128);

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 3 - Example 03\r\n");
    vSerialPutString("By Hugo Arends\r\n\r\n");

    /* Create task that samples the switches. */
    xTaskCreate( vKeyHitTask, "vKeyHitTask", configMINIMAL_STACK_SIZE, NULL, 3, NULL );

    /* Create the one shot timer, storing the handle to the created timer in xBacklightTimer. */
    /* The daemon task is set at the highest priority (in FreeRTOSConfig.h) */
    xBacklightTimer = xTimerCreate("Backlight", xBacklightOffDelay, pdFALSE, 0,
        prvBacklightTimerCallback );

    /* Check the software timers were created. */
    if( ( xBacklightTimer != NULL ) )
    {
        /* Start the scheduler. */
        vTaskStartScheduler();
    }

    /* As always, this line should not be reached. */
    for( ;; );
}

/*----------------------------------------------------------------------------*/

static void prvBacklightTimerCallback( TimerHandle_t xTimer )
{
    TickType_t xTimeNow = xTaskGetTickCount();

    /* The backlight timer expired, turn the backlight off. */
    rgb_blue_on(false);
    xSimulatedBacklightOn = pdFALSE;

    /* Print the time at which the backlight was turned off. */
    char str[64];
    sprintf(str, "Timer expired, turning backlight OFF at time\t\t% 6ld\r\n", xTimeNow);
    vSerialPutString(str);
}

/*----------------------------------------------------------------------------*/

static void vKeyHitTask( void *pvParameters )
{
    TickType_t xTimeNow;

    vSerialPutString( "Press a key to turn the backlight on.\r\n" );

    char str[64];
    char c;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    /* Ideally an application would be event driven, and use an interrupt to process key
    presses. It is not practical to use keyboard interrupts when using the FreeRTOS Windows
    port, so this task is used to poll for a key press. */
    for( ;; )
    {
        /* Has a key been pressed in the serial window? */
        if( xSerialGetChar(&c, 0) )
        {
            /* A key has been pressed. Record the time. */
            xTimeNow = xTaskGetTickCount();

            if( xSimulatedBacklightOn == pdFALSE )
            {
                /* The backlight was off, so turn it on, start the one shot timer and print,
                the time at which it was turned on. */
                rgb_blue_on(true);
                xSimulatedBacklightOn = pdTRUE;

                /* Start the software timer. A real application may read key presses in an
                interrupt. If this function was an interrupt service routine then
                xTimerResetFromISR() must be used instead of xTimerReset().
                Notice that the second parmeter is the timeout value for the timer command
                queue, not the period! */
                xTimerStart( xBacklightTimer, pdMS_TO_TICKS( 10 ) );

                sprintf(str, "Key pressed, turning backlight ON at time\t\t% 6ld\r\n", xTimeNow);
                vSerialPutString(str);
            }
            else
            {
                /* Reset the software timer. A real application may read key presses in an
                interrupt. If this function was an interrupt service routine then
                xTimerResetFromISR() must be used instead of xTimerReset(). */
                xTimerReset( xBacklightTimer, xBacklightOffDelay );

                /* The backlight was already on, so print a message to say the timer is about to
                be reset and the time at which it was reset. */
                sprintf(str, "Key pressed, resetting software timer at time\t\t% 6ld\r\n", xTimeNow);
                vSerialPutString(str);
            }
        }

        /* Sample switches 10 times per second */
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS( 100 ) );
    }
}
