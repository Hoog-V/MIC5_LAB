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
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "bitmaps.h"
#include "leds.h"
#include "rgb.h"
#include "rtc.h"
#include "serial.h"
#include "ssd1306.h"
#include "switches.h"

/*----------------------------------------------------------------------------*/
// Local defines
/*----------------------------------------------------------------------------*/
#define M_PI (3.14159265f)

typedef enum state
{
    ANALOG,
    DIGITAL,
}state_t;

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vBlinkTask(void *pvParameters);
static void vShowTask(void *pvParameters);
static void vSwTask(void *parameters);

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/
QueueHandle_t xStateQueue;

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    rgb_init();
    led_init();
    xSerialPortInit(921600, 128);

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 7 - Example 03\r\n");
    vSerialPutString("By Hugo Arends\r\n\r\n");

    xRtcOneSecondSemaphore = xSemaphoreCreateBinary();
    xRtcAlarmSemaphore = xSemaphoreCreateBinary();

    xStateQueue = xQueueCreate(1, sizeof(state_t));
    vQueueAddToRegistry(xStateQueue, "xStateQueue");

    // Create the tasks
    xTaskCreate(vBlinkTask, "Blink", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vShowTask,  "Show",  configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    xTaskCreate(vSwTask,    "Sw",    configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created.
    Chapter 2 provides more information on heap memory management. */
    for( ;; );
}

/*----------------------------------------------------------------------------*/

static void vBlinkTask(void *pvParameters)
{
    led_init();

    char str[32];
    sprintf(str, "[%*s] started\r\n", 12, __func__);
    vSerialPutString(str);

    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        led_on();
        vTaskDelay(pdMS_TO_TICKS(10));

        led_off();
        vTaskDelay(pdMS_TO_TICKS(4990));
    }
}

/*----------------------------------------------------------------------------*/

static void vShowTask(void *pvParameters)
{
    rtc_init();

    rtc_datetime_t datetime;
    datetime.year   = 2022;
    datetime.month  = 3;
    datetime.day    = 12;
    datetime.hour   = 19;
    datetime.minute = 12;
    datetime.second = 0;
    rtc_set(&datetime);

    // Wait some time so the oled display is out of reset state
    vTaskDelay(pdMS_TO_TICKS(200));

    ssd1306_init();
    ssd1306_setorientation(1);
    ssd1306_clearscreen();
    ssd1306_update();

    char str[128];
    sprintf(str, "[%*s] started\r\n", 12, __func__);
    vSerialPutString(str);

    state_t state = DIGITAL;

    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        /* Use the semaphore to wait for the event. The semaphore was created
        before the scheduler was started, so before this task ran for the first
        time. The task blocks indefinitely, meaning this function call will only
        return once the semaphore has been successfully obtained - so there is
        no need to check the value returned by xSemaphoreTake(). */
        xSemaphoreTake(xRtcOneSecondSemaphore, portMAX_DELAY);

        // State updated?
        xQueueReceive(xStateQueue, &state, 0);

        // Get time from RTC
        rtc_get(&datetime);

        // Show the time on oled display
        if(state == DIGITAL)
        {
            ssd1306_clearscreen();

            sprintf(str, "%02hd:%02hd:%02hd", datetime.hour, datetime.minute, datetime.second);
            ssd1306_setfont(Monospaced_bold_24);
            ssd1306_putstring(64-(strlen(str)*Monospaced_bold_24[0]/2),4,str);

            sprintf(str, "%02hd-%02hd-%04hd", datetime.day, datetime.month, datetime.year);
            ssd1306_setfont(Monospaced_plain_10);
            ssd1306_putstring(64-(strlen(str)*Monospaced_plain_10[0]/2),63-2*Monospaced_plain_10[1],str);

            ssd1306_update();
        }
        else if(state == ANALOG)
        {
            ssd1306_drawbitmap(clock);

            // Calculate the positions of the hands
            uint8_t x = 64 + 27.0f * cosf((datetime.second * (M_PI/30.0f)) - (M_PI/2.0f));
            uint8_t y = 31 + 27.0f * sinf((datetime.second * (M_PI/30.0f)) - (M_PI/2.0f));
            ssd1306_drawline(64, 31, x, y);

            x = 64 + 27.0f * cosf((datetime.minute * (M_PI/30.0f)) - (M_PI/2.0f));
            y = 31 + 27.0f * sinf((datetime.minute * (M_PI/30.0f)) - (M_PI/2.0f));
            ssd1306_drawline(64, 31, x, y);

            x = 64 + 20.0f * cosf((datetime.hour * (M_PI/6.0f)) - (M_PI/2.0f));
            y = 31 + 20.0f * sinf((datetime.hour * (M_PI/6.0f)) - (M_PI/2.0f));
            ssd1306_drawline(64, 31, x, y);

            ssd1306_update();
        }
    }
}

/*----------------------------------------------------------------------------*/

void vSwTask(void *pvParameters)
{
    sw_init();

    bool sw1_pressed = false;
    bool sw2_pressed = false;

    char str[32];
    sprintf(str, "[%*s] started\r\n", 12, __func__);
    vSerialPutString(str);

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for( ;; )
    {
        // Check SW1
        if(!sw1_pressed && sw_pressed(SW1))
        {
            sw1_pressed = true;
            xQueueSend(xStateQueue, &((state_t){DIGITAL}), pdMS_TO_TICKS(20));
        }

        if(sw1_pressed && !sw_pressed(SW1))
        {
            sw1_pressed = false;
        }

        // Check SW2
        if(!sw2_pressed && sw_pressed(SW2))
        {
            sw2_pressed = true;
            xQueueSend(xStateQueue, &((state_t){ANALOG}), pdMS_TO_TICKS(20));
        }

        if(sw2_pressed && !sw_pressed(SW2))
        {
            sw2_pressed = false;
        }

        // Wait before sampling the next time
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

/*----------------------------------------------------------------------------*/

static void vSyncTask(void *pvParameters)
{
    rtc_datetime_t datetime;

    char str[32];
    sprintf(str, "[%*s] started\r\n", 12, __func__);
    vSerialPutString(str);

    TickType_t xLastWakeTime = xTaskGetTickCount();

    for( ;; )
    {
        // Start synchronization every hour
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(60 * 1000));

        rtc_get(&datetime);

        if(datetime.minute >= 58)
        {
            dcf77_fix_start();
        }
    }
}
