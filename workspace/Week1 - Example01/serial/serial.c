/*
 * FreeRTOS V202107.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Updated by Hugo Arends February 2022 for the course MIC5
 * HAN University of Applied Sciences
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
	BASIC INTERRUPT DRIVEN SERIAL PORT DRIVER FOR UART0.
*/

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

/* Library includes. */
#include "MKL25Z4.h"

/* Demo application includes. */
#include "serial.h"

/*---------------------------------------------------------------------------*/

/* Misc defines. */
#define serINVALID_QUEUE    ( ( QueueHandle_t ) 0 )
#define serNO_BLOCK		    ( ( TickType_t ) 0 )
#define serTX_BLOCK_TIME    ( 40 / portTICK_PERIOD_MS )

/*---------------------------------------------------------------------------*/

/* The queues used to hold characters. */
static QueueHandle_t xRxedChars;
static QueueHandle_t xCharsForTx;
static SemaphoreHandle_t xStringMutex;

/*---------------------------------------------------------------------------*/

/*
 * See the serial.h header file.
 */
portBASE_TYPE 
xSerialPortInit( unsigned long ulWantedBaud,
                               unsigned portBASE_TYPE uxQueueLength )
{
    portBASE_TYPE xReturn = pdTRUE;

    // Create the queues used to hold Rx/Tx characters.
	xRxedChars = xQueueCreate( uxQueueLength, sizeof(char) );
	xCharsForTx = xQueueCreate( uxQueueLength + 1, sizeof(char) );

	// Create mutex
	xStringMutex = xSemaphoreCreateMutex();

	// If the queue was created correctly then setup the UART peripheral
	if( ( xRxedChars != serINVALID_QUEUE ) && ( xCharsForTx != serINVALID_QUEUE ) )
	{
        // enable clock to UART and Port A
        SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;
        SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;

        // Set UART clock to 48 MHz
        SIM->SOPT2 |= SIM_SOPT2_UART0SRC(1);
        SIM->SOPT2 |= SIM_SOPT2_PLLFLLSEL_MASK;

        // select UART pins
        PORTA->PCR[1] = PORT_PCR_ISF_MASK | PORT_PCR_MUX(2);
        PORTA->PCR[2] = PORT_PCR_ISF_MASK | PORT_PCR_MUX(2);

        UART0->C2 &=  ~(UARTLP_C2_TE_MASK | UARTLP_C2_RE_MASK);

        // Set baud rate to baud rate
        uint32_t divisor = 48000000UL/(ulWantedBaud*16);
        UART0->BDH = UART_BDH_SBR(divisor>>8);
        UART0->BDL = UART_BDL_SBR(divisor);

        // No parity, 8 bits, one stop bit, other settings;
        UART0->C1 = 0;
        UART0->S2 = 0;
        UART0->C3 = 0;

        // Enable transmitter and receiver but not interrupts
        UART0->C2 = UART_C2_TE_MASK | UART_C2_RE_MASK;

        // Enable the interrupt in the NVIC
        NVIC_SetPriority(UART0_IRQn, 128);
        NVIC_ClearPendingIRQ(UART0_IRQn);
        NVIC_EnableIRQ(UART0_IRQn);

        // Enable receive interrupts
        UART0->C2 |= UART_C2_RIE_MASK;
	}
	else
	{
		xReturn = pdFALSE;
	}

	return xReturn;
}

/*---------------------------------------------------------------------------*/

portBASE_TYPE xSerialPutChar( char cOutChar, TickType_t xBlockTime )
{
    portBASE_TYPE xReturn;

    // Send the character to the queue. Return false if xBlockTime expires.
    if( xQueueSend( xCharsForTx, &cOutChar, xBlockTime ) == pdPASS )
    {
        xReturn = pdPASS;
        UART0->C2 |= UART_C2_TIE_MASK;
    }
    else
    {
        xReturn = pdFAIL;
    }

    return xReturn;
}

/*---------------------------------------------------------------------------*/

portBASE_TYPE xSerialGetChar( char *pcRxedChar, TickType_t xBlockTime )
{
	// Get the next character from the buffer.  Return false if no characters
	// are available, or arrive before xBlockTime expires.
	if( xQueueReceive( xRxedChars, pcRxedChar, xBlockTime ) )
	{
		return pdTRUE;
	}
	else
	{
		return pdFALSE;
	}
}

/*---------------------------------------------------------------------------*/

void vSerialPutString( const char * const pcString )
{
    // NOTE: This implementation does not handle the queue being full as no
    // block time is used!

    char *pxNext = (char * const)pcString;

    // Attempt to take the mutex, blocking indefinitely to wait for the mutex
    // if it is not available straight away. The call to xSemaphoreTake() will
    // only return when the mutex has been successfully obtained, so there is
    // no need to check the function return value. If any other delay period
    // was used then the code must check that xSemaphoreTake() returns pdTRUE
    // before accessing the shared resource (which in this case is standard
    // out). As noted earlier in this book, indefinite time outs are not
    // recommended for production code.
    xSemaphoreTake(xStringMutex, portMAX_DELAY);
    {
        while(*pxNext)
        {
            xSerialPutChar(*pxNext, serNO_BLOCK);
            pxNext++;
        }
    }
    xSemaphoreGive(xStringMutex);
}

/*---------------------------------------------------------------------------*/

void UART0_IRQHandler( void )
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    char cChar;

	if( UART0->S1 & UART_S1_TDRE_MASK )
	{
		// The interrupt was caused by the data register becoming empty.
		// Are there any more characters to transmit?
		if( xQueueReceiveFromISR( xCharsForTx, &cChar, &xHigherPriorityTaskWoken ) == pdTRUE )
		{
			// A character was retrieved from the transmit queue so send it.
			UART0->D = cChar;
		}
		else
		{
		    // No more characters in the transmit queue, disable transmit
		    // interrupt.
			UART0->C2 &= ~UART_C2_TIE_MASK;
		}
	}

	if( UART0->S1 & UART_S1_RDRF_MASK )
	{
        // The interrupt was caused by incoming data. Read the data and store
	    // in the receive queue.
		cChar = UART0->D;
		xQueueSendFromISR( xRxedChars, &cChar, &xHigherPriorityTaskWoken );
	}

    // Pass the xHigherPriorityTaskWoken value into portEND_SWITCHING_ISR(). If
	// xHigherPriorityTaskWoken was set to pdTRUE inside one of the ...FromISR()
	// functions then calling portEND_SWITCHING_ISR() will request a context switch.
	// If xHigherPriorityTaskWoken is still pdFALSE then calling
	// portEND_SWITCHING_ISR() will have no effect.
	portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}
