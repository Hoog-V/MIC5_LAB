/*! ***************************************************************************
 *
 * \brief     Low level driver for the switches
 * \file      switches.c
 * \author    Hugo Arends
 * \date      June 2021
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
#include "switches.h"

static PORT_Type * port_mapping[N_SWITCHES] = {PORTD, PORTD};
static GPIO_Type * gpio_mapping[N_SWITCHES] = {PTD,   PTD};
static uint8_t     pin_mapping[N_SWITCHES]  = {3,     5};

/*!
 * \brief Initialises the switches on the shield
 *
 * This functions initializes the switches on the shield.
 */
void sw_init(void)
{
    // Enable clocks to PORT
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
    
    // Configure all pins as follows:
    // - MUX[2:0] = 001 : Alternative 1 (GPIO)
    // - DSE = 0 : Low drive strength
    // - PFE = 0 : Passive input filter is disabled
    // - SRE = 0 : Fast slew rate is configured
    // - PE = 1 : Internal pullup or pulldown resistor is enabled
    // - PS = 1 : Internal pullup resistor is enabled
    for(int i=0; i<N_SWITCHES; i++)
    {
        port_mapping[i]->PCR[pin_mapping[i]] = 0b00100000011;
    }
    
    // Set port pins to inputs
    for(int i=0; i<N_SWITCHES; i++)
    {
        gpio_mapping[i]->PDDR &= ~(1<<pin_mapping[i]);
    }
}

/*!
 * \brief Check if a switch is pressed
 *
 * This functions checks if a switch is pressed. The function simply checks the
 * value of the switch at the moment the function is called. It doesn't
 * remember if the switch has been pressed.
 *
 * \param[in]  sw  Switch that will be checked, must be of type ::sw_t
 *
 * \return True if the switch is pressed, false otherwise
 */
bool sw_pressed(const sw_t sw)
{
    // If the key is pressed, the bit at that position will read logic 0
    return ((gpio_mapping[sw]->PDIR & (1<<pin_mapping[sw])) == 0);
}
