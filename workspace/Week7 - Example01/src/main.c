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

#include "leds.h"
#include "rgb.h"
#include "rtc.h"
#include "ssd1306.h"
#include "serial.h"
#include "switches.h"
#include "tcrt5000.h"

/*----------------------------------------------------------------------------*/
// Local defines
/*----------------------------------------------------------------------------*/
#define mainTOTAL_CYCLE_MS (1000)
#define mainN_SLOTS        (20)
#define mainSLOT_MS        (mainTOTAL_CYCLE_MS / mainN_SLOTS)

/*----------------------------------------------------------------------------*/
// Local type definitions
/*----------------------------------------------------------------------------*/
typedef enum
{
    UP,
    DOWN,
}command_t;

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/
static void vLedOnTask(void *parameters);
static void vLedOffTask(void *parameters);
static void vOledTask(void *parameters);
static void vIrTask(void *parameters);
static void vDtTask(void *parameters);
static void vSwTask(void *parameters);
static void vTsiTask(void *parameters);
static void vCmdTask(void *parameters);

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/
static SemaphoreHandle_t xOledMutex;

static QueueHandle_t xCmdQueue;

static TaskHandle_t vLedOnTaskHandle;
static TaskHandle_t vLedOffTaskHandle;
static TaskHandle_t vOledTaskHandle;
static TaskHandle_t vIrTaskHandle;
static TaskHandle_t vDtTaskHandle;
static TaskHandle_t vSwTaskHandle;
static TaskHandle_t vTsiTaskHandle;
static TaskHandle_t vCmdTaskHandle;

static volatile uint32_t ulRunningTaskNum = 0;

extern uint8_t FreeRTOSDebugConfig[];

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    // Make sure this variable is not optimized out by the linker. If this is
    // Optimized out, the debugger doens't show the tasks as separate threads
    if(FreeRTOSDebugConfig[0] == 0)
    {
        for(;;);
    }

    rgb_init();
    led_init();
    tcrt5000_init();
    rtc_init();
    sw_init();
    xSerialPortInit(921600, 128);

    rtc_datetime_t datetime;
    datetime.year   = 2022U;
    datetime.month  = 2U;
    datetime.day    = 22U;
    datetime.hour   = 12U;
    datetime.minute = 0;
    datetime.second = 0;
    rtc_set(&datetime);

    ssd1306_init();
    ssd1306_setorientation(1);
    ssd1306_setfont(Monospaced_plain_12);
    ssd1306_clearscreen();

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS demo Week 7 - Example 01\r\n");
    vSerialPutString("By Hugo Arends\r\n\r\n");

//    xTaskCreate(vMonitor,  "vMonitor",  configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(vLedOnTask,  "vLedOnTask",    configMINIMAL_STACK_SIZE, NULL, 1, &vLedOnTaskHandle);
    xTaskCreate(vLedOffTask, "vLedOffTask",   configMINIMAL_STACK_SIZE, NULL, 1, &vLedOffTaskHandle);
    xTaskCreate(vOledTask,   "vOledTask",     configMINIMAL_STACK_SIZE, NULL, 1, &vOledTaskHandle);
    xTaskCreate(vIrTask,     "vIrTask",       configMINIMAL_STACK_SIZE, NULL, 1, &vIrTaskHandle);
    xTaskCreate(vDtTask,     "vDtTask",     2*configMINIMAL_STACK_SIZE, NULL, 1, &vDtTaskHandle);
    xTaskCreate(vSwTask,     "vSwTask",       configMINIMAL_STACK_SIZE, NULL, 1, &vSwTaskHandle);
    xTaskCreate(vTsiTask,    "vTsiTask",      configMINIMAL_STACK_SIZE, NULL, 1, &vTsiTaskHandle);
    xTaskCreate(vCmdTask,    "vCmdTask",      configMINIMAL_STACK_SIZE, NULL, 1, &vCmdTaskHandle);

    xOledMutex = xSemaphoreCreateMutex();
    xRtcOneSecondSemaphore = xSemaphoreCreateBinary();
    xRtcAlarmSemaphore = xSemaphoreCreateBinary();
    xCmdQueue = xQueueCreate(10, sizeof(command_t));

    vQueueAddToRegistry(xOledMutex, "xOledMutex");
    vQueueAddToRegistry(xRtcOneSecondSemaphore, "xOneSecondSemaphore");
    vQueueAddToRegistry(xRtcAlarmSemaphore, "xRtcAlarmSemaphore");
    vQueueAddToRegistry(xCmdQueue, "xCmdQueue");

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created.
    Chapter 2 provides more information on heap memory management. */
    for( ;; );
}

void vApplicationIdleHook( void )
{
    ulRunningTaskNum = 0;
}

/*----------------------------------------------------------------------------*/

//static void vMonitor(void *parameters)
//{
//    TickType_t xLastWakeTime = xTaskGetTickCount();
//    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
//
//    eTaskState taskState = eInvalid;
//
//    /* As per most tasks, this task is implemented within an infinite loop. */
//    for( ;; )
//    {
//        taskState = eTaskGetState(vTask00Handle);
//
//        // Go into Blocking state for one slot time
//        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(mainSLOT_MS));
//    }
//}

/*----------------------------------------------------------------------------*/

static void vLedOnTask(void *parameters)
{
    // Sending a maximum of 24 characters takes
    // 24 x 10 bits x 1/921600 s = 0.26 ms
    char str[24];

    TickType_t xLastWakeTime = xTaskGetTickCount();
//    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(0 * mainSLOT_MS));

    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        // For debugging: show info
        sprintf(str, "% 7u | %s\r\n", xLastWakeTime, __func__);
        vSerialPutString(str);

        if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(20)) == pdPASS)
        {
            ssd1306_setfont(Monospaced_plain_12);
            ssd1306_putstring(0, 0, str);
            xSemaphoreGive(xOledMutex);
        }

        // Do work
        led_on();

        // Go into Blocking state for one cycle total time
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(mainTOTAL_CYCLE_MS));
        ulRunningTaskNum = 1;
    }
}

/*----------------------------------------------------------------------------*/

static void vLedOffTask(void *parameters)
{
    char str[24];

    TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1 * mainSLOT_MS));

    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        // For debugging: show info
        sprintf(str, "% 7u | %s\r\n", xLastWakeTime, __func__);
        vSerialPutString(str);

        if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(20)) == pdPASS)
        {
            ssd1306_setfont(Monospaced_plain_12);
            ssd1306_putstring(0, 0, str);
            xSemaphoreGive(xOledMutex);
        }

        // Do work
        led_off();

        // Go into Blocking state for one cycle total time
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(mainTOTAL_CYCLE_MS));
        ulRunningTaskNum = 2;
    }
}

/*----------------------------------------------------------------------------*/

static void vOledTask(void *parameters)
{
    char str[24];

    TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(4 * mainSLOT_MS));

    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        // For debugging: show info
        sprintf(str, "% 7u | %s\r\n", xLastWakeTime, __func__);
        vSerialPutString(str);

        // Do work
        ssd1306_update();

        // Go into Blocking state for one cycle total time
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(mainTOTAL_CYCLE_MS / 4));
        ulRunningTaskNum = 5;
    }
}

/*----------------------------------------------------------------------------*/

static void vIrTask(void *parameters)
{
    char str[24];

    TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2 * mainSLOT_MS));

    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        // For debugging: show info
        sprintf(str, "% 7u | %s\r\n", xLastWakeTime, __func__);
        vSerialPutString(str);

        // Do work

        // - AIEN = 0     : Conversion complete interrupt is disabled
        // - DIFF = 0     : Single-ended conversions and input channels are
        //                  selected
        // - ADCH = 01000 : Channel 8
        ADC0->SC1[0] = ADC_SC1_ADCH(8);

        // Wait for conversion to complete
        while(!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
        {;}

        // Read the result and complement
        uint16_t off_brightness = 0xFFFF - ADC0->R[0];

        // IR LED on
        PTA->PCOR = (1<<16);

        // Delay to settle down the signal
        vTaskDelay(pdMS_TO_TICKS(1));

        // - AIEN = 0     : Conversion complete interrupt is disabled
        // - DIFF = 0     : Single-ended conversions and input channels are
        //                  selected
        // - ADCH = 01000 : Channel 8
        ADC0->SC1[0] = ADC_SC1_ADCH(8);

        // Wait for conversion to complete
        while(!(ADC0->SC1[0] & ADC_SC1_COCO_MASK))
        {;}

        // Read the result and complement
        uint16_t on_brightness = 0xFFFF - ADC0->R[0];

        // IR LED off
        PTA->PSOR = (1<<16);

        // Process the ADC result
        int32_t result = (int32_t)on_brightness - (int32_t)off_brightness;
        rgb_green_on(result < 2000);
        rgb_red_on(result >= 2000);

        // Go into Blocking state for one cycle total time
//        sprintf(str, "% 7u | %s Blocking\r\n", xTaskGetTickCount(), __func__);
//        vSerialPutString(str);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(mainTOTAL_CYCLE_MS / 4));
        ulRunningTaskNum = 3;
    }
}

/*----------------------------------------------------------------------------*/

static void vDtTask(void *pvParameters)
{
    rtc_datetime_t datetime;
    char str[128];

    TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(5 * mainSLOT_MS));

    /* As per most tasks, this task is implemented within an infinite loop. */
    for( ;; )
    {
        // For debugging: show info
        sprintf(str, "% 7u | %s\r\n", xLastWakeTime, __func__);
        vSerialPutString(str);

        if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(20)) == pdPASS)
        {
            ssd1306_putstring(0, 0, str);
            xSemaphoreGive(xOledMutex);
        }

        if(xSemaphoreTake(xRtcOneSecondSemaphore, 0) == pdTRUE)
        {
            rtc_get(&datetime);
            sprintf(str, "%04hd-%02hd-%02hd " \
                         "%02hd:%02hd:%02d",
                         datetime.year, datetime.month, datetime.day,
                         datetime.hour, datetime.minute, datetime.second);

            if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(20)) == pdPASS)
            {
                ssd1306_setfont(Monospaced_plain_10);
                ssd1306_putstring(0, 51, str);
                xSemaphoreGive(xOledMutex);
            }
        }

        // Go into Blocking state for one cycle total time
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(mainTOTAL_CYCLE_MS));
        ulRunningTaskNum = 6;
    }
}

/*----------------------------------------------------------------------------*/

static void vSwTask(void *parameters)
{
    char str[24];

    const command_t command_up = UP;
    const command_t command_down = DOWN;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(3 * mainSLOT_MS));

    for( ;; )
    {
        // For debugging: show info
        sprintf(str, "% 7u | %s\r\n", xLastWakeTime, __func__);
        vSerialPutString(str);

        if(sw_pressed(SW1))
        {
            xQueueSend(xCmdQueue, &command_down, pdMS_TO_TICKS(10));
        }

        if(sw_pressed(SW2))
        {
            xQueueSend(xCmdQueue, &command_up, pdMS_TO_TICKS(10));
        }

        // Go into Blocking state for one cycle total time
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(mainTOTAL_CYCLE_MS / 2));
        ulRunningTaskNum = 4;
    }
}

/*----------------------------------------------------------------------------*/

static void vTsiTask(void *parameters)
{
    char str[24];

    const command_t command_up = UP;
    const command_t command_down = DOWN;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(8 * mainSLOT_MS));

    // Enable PTB clock
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    // Set pin to FTM
    // PTB16: TSI0_CH9, Mux Alt 0 (default)
    // PTB17: TSI0_CH10, Mux Alt 0 (default)
    PORTB->PCR[16] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[16] |= PORT_PCR_MUX(0);
    PORTB->PCR[17] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[17] |= PORT_PCR_MUX(0);

    // Enable TSI Clock
    SIM->SCGC5 |= SIM_SCGC5_TSI_MASK;

    // Setup general control and status register
    // - OUTRGF : 1 - Clear Out-of-range flag
    // - ESOR   : 0 - Out-of-range interrupt is allowed (default)
    // - MODE   : 0000 - Set TSI in capacitive sensing(non-noise detection) mode
    //                   (default)
    // - REFCHRG: 000 - 500 nA reference oscillator charge and discharge current
    //                  value (default)
    // - DVOLT  : 00 - DV = 1.03 V; V P = 1.33 V; V m = 0.30 V oscillator's
    //                 voltage rails (default)
    // - EXTCHRG: 000 - 500 nA electrode oscillator charge and discharge current
    //                  (default)
    // - PS     : 0 - Electrode Oscillator Frequency divided by 1 prescaler
    //                (default)
    // - NSCN   : 11111 - 32 scans for each electrode
    // - TSIEN  : 1 - TSI module enabled
    // - TSIIEN : 0 - TSI interrupt is disabled (default)
    // - STPE   : 1 - Allow TSI to continue running in all low power modes
    // - STM    : 0 - Software trigger scan (default)
    // - SCNIP  : n/a
    // - EOSF   : 1 - Clear scan complete flag
    // - CURSW  : 0 - The current source pair are not swapped (default)
    TSI0->GENCS =
        TSI_GENCS_OUTRGF(1) |
        TSI_GENCS_NSCN(31) |
        TSI_GENCS_TSIEN(1) |
        TSI_GENCS_STPE(1) |
        TSI_GENCS_EOSF(1);

    uint32_t channel09_result;
    uint32_t channel10_result;

    /* As per most tasks, this task is implemented in an infinite loop. */
    for( ;; )
    {
        // For debugging: show info
        sprintf(str, "% 7u | %s\r\n", xLastWakeTime, __func__);
        vSerialPutString(str);

        // ------------------------------------------------------------------------
        // Scan channel 9
        // ------------------------------------------------------------------------

        // Select channel 9
        TSI0->DATA = TSI_DATA_TSICH(9);

        // Start a scan
        TSI0->DATA |= TSI_DATA_SWTS_MASK;

        // Wait for scan complete
        while(!(TSI0->GENCS & TSI_GENCS_EOSF_MASK))
        {;}

        // Read TSI Conversion Counter Value, masking all other bits
        channel09_result = TSI0->DATA & TSI_DATA_TSICNT_MASK;

        // Clear scan complete flag
        TSI0->GENCS |= TSI_GENCS_EOSF_MASK;

        // Wait some time before sampling the next channel
        vTaskDelay(pdMS_TO_TICKS(1));

        // ------------------------------------------------------------------------
        // Scan channel 10
        // ------------------------------------------------------------------------

        // Select channel 10
        TSI0->DATA = TSI_DATA_TSICH(10);

        // Start a scan
        TSI0->DATA |= TSI_DATA_SWTS_MASK;

        // Wait for scan complete
        while(!(TSI0->GENCS & TSI_GENCS_EOSF_MASK))
        {;}

        // Read TSI Conversion Counter Value, masking all other bits
        channel10_result = TSI0->DATA & TSI_DATA_TSICNT_MASK;

        // Clear scan complete flag
        TSI0->GENCS |= TSI_GENCS_EOSF_MASK;

        // ------------------------------------------------------------------------
        // Calculate the scan results
        // ------------------------------------------------------------------------
        // The variables channel09_result and channel10_result now hold the
        // accumulated scan counter values (32 scans as set by TSI_GENCS_NSCN).
        // The following threshold value is determined by using the debugger.
        const uint32_t threshold = 0x00000380;

        // Send command according the scan results
        if(channel09_result > threshold)
        {
            xQueueSend(xCmdQueue, &command_down, pdMS_TO_TICKS(10));
        }

        if(channel10_result > threshold)
        {
            xQueueSend(xCmdQueue, &command_up, pdMS_TO_TICKS(10));
        }

        // Go into Blocking state for one cycle total time
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(mainTOTAL_CYCLE_MS / 2));
        ulRunningTaskNum = 9;
    }
}

/*----------------------------------------------------------------------------*/

static void vCmdTask(void *parameters)
{
    // Sending a maximum of 24 characters takes
    // 24 x 10 bits x 1/921600 s = 0.26 ms
    char str[24];

    TickType_t xLastWakeTime = xTaskGetTickCount();
    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10 * mainSLOT_MS));

    command_t command;

    for( ;; )
    {
        // For debugging: show info
        sprintf(str, "% 7u | %s\r\n", xLastWakeTime, __func__);
        vSerialPutString(str);

        if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(20)) == pdPASS)
        {
            ssd1306_setfont(Monospaced_plain_12);
            ssd1306_putstring(0, 15, "    ");
            xSemaphoreGive(xOledMutex);
        }

        while(xQueueReceive(xCmdQueue, &command, 0))
        {
            if(xSemaphoreTake(xOledMutex, pdMS_TO_TICKS(20)) == pdPASS)
            {
                char *cmd_str = (command == UP) ? "Up  " : "Down";
                ssd1306_setfont(Monospaced_plain_12);
                ssd1306_putstring(0, 15, cmd_str);
                xSemaphoreGive(xOledMutex);
            }
        }

        // Go into Blocking state for one cycle total time
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(mainTOTAL_CYCLE_MS));
        ulRunningTaskNum = 11;
    }
}
