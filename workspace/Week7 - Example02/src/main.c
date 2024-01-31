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
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "leds.h"
#include "mma8451.h"
#include "rgb.h"
#include "serial.h"
#include "ssd1306.h"
#include "switches.h"
#include "tcrt5000.h"

/*----------------------------------------------------------------------------*/
// Local defines
/*----------------------------------------------------------------------------*/
typedef struct
{
    int16_t x;
    int16_t y;
}point_t;

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vBlinkTask(void *pvParameters);
static void vMma8451Task(void *pvParameters);
static void vOledTask(void *parameters);
static void vDrawTask(void *parameters);
static void vSwTask(void *parameters);
static void vADCTask(void *parameters);

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/
TaskHandle_t xMma8451TaskHandler;
static SemaphoreHandle_t xOledMutex;
static QueueHandle_t xCircleQueue;

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    rgb_init();
    xSerialPortInit(921600, 128);

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 7 - Example 02\r\n");
    vSerialPutString("By Hugo Arends\r\n\r\n");

    // Create the tasks
    xTaskCreate(vBlinkTask,   "Blink",   configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vMma8451Task, "MMA4851", configMINIMAL_STACK_SIZE, NULL, 4, &xMma8451TaskHandler);
    xTaskCreate(vOledTask,    "Oled",    configMINIMAL_STACK_SIZE, NULL, 3, NULL);
    xTaskCreate(vDrawTask,    "Draw",    configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(vSwTask,      "Sw",      configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vADCTask,     "ADC",     configMINIMAL_STACK_SIZE, NULL, 2, &xADCTaskHandle);

    // Craeate mutex for accessing oled display
    xOledMutex = xSemaphoreCreateMutex();
    vQueueAddToRegistry(xOledMutex, "xOledMutex");

    // Create queue for circle location
    xCircleQueue = xQueueCreate(5, sizeof(point_t));
    vQueueAddToRegistry(xCircleQueue, "xCircleQueue");

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created.
    Chapter 2 provides more information on heap memory management. */
    for( ;; );
}

/*----------------------------------------------------------------------------*/

static void vBlinkTask( void *pvParameters )
{
    led_init();

    char str[32];
    sprintf(str, "[%*s] started\r\n", 12, __func__);
    vSerialPutString(str);

    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        led_on();
        vTaskDelay(pdMS_TO_TICKS(50));

        led_off();
        vTaskDelay(pdMS_TO_TICKS(1950));
    }
}

/*----------------------------------------------------------------------------*/

static void vMma8451Task( void *pvParameters )
{
    if(!mma8451_init())
    {
        vSerialPutString("mma8451 init failed\r\n");
    }
    else
    {
        if(!mma8451_calibrate())
        {
            vSerialPutString("mma8451 calibrate failed\r\n");
        }
    }

    point_t point = {64, 32};
    float x = 64.0f;
    float y = 32.0f;

    char str[32];
    sprintf(str, "[%*s] started\r\n", 12, __func__);
    vSerialPutString(str);

    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        // Wait for notification
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);

        // These functions are NOT reentrent and should be called by a single
        // task
        mma8451_read();
        mma8451_rollpitch();

        // Limit the inputs
        roll  = ((roll >= -10) && (roll <= 10)) ? 0 : roll;
        pitch = ((pitch >= -10) && (pitch <= 10)) ? 0 : pitch;

        // Calculate next center
        x = x + (roll / 100);
        y = y + (pitch / 100);

        // Limit the results
        x = (x > 127) ? 127 : x;
        x = (x <   0) ?   0 : x;

        y = (y > 63) ? 63 : y;
        y = (y <  0) ?  0 : y;

        point.x = (int16_t)x;
        point.y = (int16_t)y;
        xQueueSend(xCircleQueue, &point, pdMS_TO_TICKS(10));

//        char str[32];
//        sprintf(str, "roll : %5.1f\tpitch: %5.1f\r", roll, pitch);
//        vSerialPutString(str);
    }
}

/*----------------------------------------------------------------------------*/

static void vOledTask(void *parameters)
{
    ssd1306_init();
    ssd1306_setorientation(1);
    ssd1306_setfont(Monospaced_plain_12);
    ssd1306_clearscreen();
    ssd1306_putstring(0, 0, "FreeRTOS demo");
    ssd1306_putstring(0,15, "project");
    ssd1306_update();

    char str[32];
    sprintf(str, "[%*s] started\r\n", 12, __func__);
    vSerialPutString(str);

    // Show welcome message for one second
    vTaskDelay(pdMS_TO_TICKS(1000));

    if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(100)) == pdPASS)
    {
        ssd1306_clearscreen();
        xSemaphoreGive(xOledMutex);
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();

    // As per most tasks, this task is implemented in an infinite loop.
    for( ;; )
    {
        ssd1306_update();

        // Wait before updating the next time
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

/*----------------------------------------------------------------------------*/

static void vDrawTask(void *parameters)
{
    point_t point1 = {64, 32};
    point_t point2 = {64, 32};

    char str[32];
    sprintf(str, "[%*s] started\r\n", 12, __func__);
    vSerialPutString(str);

    // As per most tasks, this task is implemented in an infinite loop.
    for( ;; )
    {
        if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(20)) == pdPASS)
        {
            point2.x = (point2.x > 125) ? 125 : point2.x;
            point2.x = (point2.x <   2) ?   2 : point2.x;

            point2.y = (point2.y > 61) ? 61 : point2.y;
            point2.y = (point2.y <  2) ?  2 : point2.y;

            // Remove old circle
            ssd1306_setpixel(point1.x-2, point1.y,   OFF);
            ssd1306_setpixel(point1.x-2, point1.y-1, OFF);
            ssd1306_setpixel(point1.x-2, point1.y+1, OFF);
            ssd1306_setpixel(point1.x+2, point1.y,   OFF);
            ssd1306_setpixel(point1.x+2, point1.y-1, OFF);
            ssd1306_setpixel(point1.x+2, point1.y+1, OFF);
            ssd1306_setpixel(point1.x,   point1.y-2, OFF);
            ssd1306_setpixel(point1.x-1, point1.y-2, OFF);
            ssd1306_setpixel(point1.x+1, point1.y-2, OFF);
            ssd1306_setpixel(point1.x,   point1.y+2, OFF);
            ssd1306_setpixel(point1.x-1, point1.y+2, OFF);
            ssd1306_setpixel(point1.x+1, point1.y+2, OFF);

            // Draw new circle
            ssd1306_setpixel(point2.x-2, point2.y,   ON);
            ssd1306_setpixel(point2.x-2, point2.y-1, ON);
            ssd1306_setpixel(point2.x-2, point2.y+1, ON);
            ssd1306_setpixel(point2.x+2, point2.y,   ON);
            ssd1306_setpixel(point2.x+2, point2.y-1, ON);
            ssd1306_setpixel(point2.x+2, point2.y+1, ON);
            ssd1306_setpixel(point2.x,   point2.y-2, ON);
            ssd1306_setpixel(point2.x-1, point2.y-2, ON);
            ssd1306_setpixel(point2.x+1, point2.y-2, ON);
            ssd1306_setpixel(point2.x,   point2.y+2, ON);
            ssd1306_setpixel(point2.x-1, point2.y+2, ON);
            ssd1306_setpixel(point2.x+1, point2.y+2, ON);

            xSemaphoreGive(xOledMutex);
        }

        point1 = point2;

        // Wait until a new circle must be drawn
        xQueueReceive(xCircleQueue, &point2, portMAX_DELAY);
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

            if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(20)) == pdPASS)
            {
                ssd1306_putstring(0, 25, " SW1 was pressed ");
                xSemaphoreGive(xOledMutex);
            }
        }

        if(sw1_pressed && !sw_pressed(SW1))
        {
            sw1_pressed = false;
        }

        // Check SW2
        if(!sw2_pressed && sw_pressed(SW2))
        {
            sw2_pressed = true;

            if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(20)) == pdPASS)
            {
                ssd1306_clearscreen();
                xSemaphoreGive(xOledMutex);
            }
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

static void vADCTask(void *parameters)
{
    tcrt5000_init();

    // Enable the second line to see what happens if this task is not notified
    // within the expected timeout time
    const TickType_t xADCConversionTimeout = pdMS_TO_TICKS(110);

    uint32_t ulADCResult;
    BaseType_t xResult;

    char str[32];
    sprintf(str, "[%*s] started\r\n", 12, __func__);
    vSerialPutString(str);

    // As per most tasks, this task is implemented in an infinite loop.
    for( ;; )
    {
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

            // Process the ADC result
            rgb_green_on(ulADCResult < 2000);
            rgb_red_on(ulADCResult >= 2000);
        }
        else
        {
            // No notification received (timed out)
            sprintf(str, "[%*s] No notification within expected time\r\n", 12, __func__);
            vSerialPutString(str);
        }
    }
}
