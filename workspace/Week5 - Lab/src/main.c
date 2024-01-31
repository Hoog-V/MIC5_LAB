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
#include "semphr.h"
#include "task.h"

#include "ssd1306.h"
#include "rgb.h"
#include "serial.h"
#include "switches.h"

/*----------------------------------------------------------------------------*/
// Local type definitions
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// Local function prototypes
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// Local variables
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
// Main application
/*----------------------------------------------------------------------------*/
int main(void)
{
    rgb_init();
    sw_init();
    ssd1306_init();
    xSerialPortInit(921600, 128);

    vSerialPutString("\r\nFRDM-KL25Z FreeRTOS Week 5 - Lab\r\n\r\n");

    // Set initial LCD message
    ssd1306_setorientation(1);
    ssd1306_setfont(Monospaced_plain_12);
    ssd1306_clearscreen();
    ssd1306_putstring(0, 0, "Week 5 - Lab");
    ssd1306_update();

    vSerialPutString("[main      ] Starting scheduler\r\n");

    /* Start the scheduler so the tasks start executing. */
    vTaskStartScheduler();

    /* If all is well then main() will never reach here as the scheduler will
    now be running the tasks. If main() does reach here then it is likely that
    there was insufficient heap memory available for the idle task to be created.
    Chapter 2 provides more information on heap memory management. */
    for( ;; );
}

/*----------------------------------------------------------------------------*/