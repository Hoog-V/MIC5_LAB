// THIS RTC EXAMPLE REQUIRES AN EXTERNAL CONNECTION BETWEEN PTC1 AND PTC3.
// Refer to https://community.nxp.com/docs/DOC-94734

#include "rtc.h"
#include "datetime.h"

SemaphoreHandle_t xRtcOneSecondSemaphore;
SemaphoreHandle_t xRtcAlarmSemaphore;

void rtc_init(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;

    // Refer to: https://community.nxp.com/docs/DOC-94734

    // Enable the internal reference clock. MCGIRCLK is active.
    MCG->C1 |= MCG_C1_IRCLKEN_MASK;

    // Select the slow internal reference clock source.
    MCG->C2 &= ~(MCG_C2_IRCS_MASK);

    // Set PTC1 as RTC_CLKIN and select 32 KHz clock source for the RTC module.
    PORTC->PCR[1] |= (PORT_PCR_MUX(0x1));
    SIM->SOPT1 &= ~SIM_SOPT1_OSC32KSEL_MASK;
    SIM->SOPT1 |= SIM_SOPT1_OSC32KSEL(0b10);

    // Set PTC3 as CLKOUT pin and selects the MCGIRCLK clock to output on the
    // CLKOUT pin.
    SIM->SOPT2 |= SIM_SOPT2_CLKOUTSEL(0b100);
    PORTC->PCR[3] |= (PORT_PCR_MUX(0x5));


    // Enable software access and interrupts to the RTC module.
    SIM->SCGC6 |= SIM_SCGC6_RTC_MASK;

    // Clear all RTC registers.
    RTC->CR = RTC_CR_SWR_MASK;
    RTC->CR &= ~RTC_CR_SWR_MASK;

    if(RTC_SR & RTC_SR_TIF_MASK)
    {
        RTC_TSR = 0x00000000;
    }

    // Set time compensation parameters. (These parameters can be different for
    // each application)
    RTC->TCR = RTC_TCR_CIR(0) | RTC_TCR_TCR(0);

    // Enable time seconds interrupt for the module and enable its IRQ.
    NVIC_SetPriority(RTC_Seconds_IRQn, 64);
    NVIC_ClearPendingIRQ(RTC_Seconds_IRQn);
    NVIC_EnableIRQ(RTC_Seconds_IRQn);

    RTC->IER |= RTC_IER_TSIE_MASK;

    // Enable alarm interrupt for the module and enable its IRQ.
    NVIC_SetPriority(RTC_IRQn, 64);
    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);

    RTC->IER |= RTC_IER_TAIE_MASK;

    // Enable time counter.
    RTC->SR |= RTC_SR_TCE_MASK;

    // Write to Time Seconds Register.
  //RTC_TSR = 0xFF;
}

void rtc_get(rtc_datetime_t *datetime)
{
    // Read the number of seconds from the RTC Time Seconds Register
    uint32_t seconds = RTC->TSR;

    RTC_HAL_ConvertSecsToDatetime(&seconds, datetime);
}

void rtc_set(rtc_datetime_t *datetime)
{
    uint32_t seconds;
    RTC_HAL_ConvertDatetimeToSecs(datetime, &seconds);

    // Write the number of seconds to the RTC Time Seconds Register
    RTC->SR &= ~RTC_SR_TCE_MASK;
    RTC->TSR = seconds;
    RTC->SR |= RTC_SR_TCE_MASK;
}

void RTC_IRQHandler(void)
{
    // Clear pending interrupts
    NVIC_ClearPendingIRQ(RTC_IRQn);

    if(RTC->SR & RTC_SR_TAF_MASK)
    {
        // Clear the TAF flag
        RTC->TAR = 0;

        /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
        it will get set to pdTRUE inside the interrupt safe API function if a
        context switch is required. */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        /* 'Give' the semaphore. This will unblock the deferred interrupt handling
        task */
        xSemaphoreGiveFromISR( xRtcAlarmSemaphore, &xHigherPriorityTaskWoken );

        /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR(). If
        xHigherPriorityTaskWoken was set to pdTRUE inside xSemaphoreGiveFromISR()
        then calling portYIELD_FROM_ISR() will request a context switch. If
        xHigherPriorityTaskWoken is still pdFALSE then calling
        portYIELD_FROM_ISR() will have no effect. */
        portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
}

void RTC_Seconds_IRQHandler(void)
{
    // Clear pending interrupts
    NVIC_ClearPendingIRQ(RTC_Seconds_IRQn);

    /* The xHigherPriorityTaskWoken parameter must be initialized to pdFALSE as
    it will get set to pdTRUE inside the interrupt safe API function if a
    context switch is required. */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* 'Give' the semaphore. This will unblock the deferred interrupt handling
    task */
    xSemaphoreGiveFromISR( xRtcOneSecondSemaphore, &xHigherPriorityTaskWoken );

    /* Pass the xHigherPriorityTaskWoken value into portYIELD_FROM_ISR(). If
    xHigherPriorityTaskWoken was set to pdTRUE inside xSemaphoreGiveFromISR()
    then calling portYIELD_FROM_ISR() will request a context switch. If
    xHigherPriorityTaskWoken is still pdFALSE then calling
    portYIELD_FROM_ISR() will have no effect. */
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
