// THIS RTC EXAMPLE REQUIRES AN EXTERNAL CONNECTION BETWEEN PTC1 AND PTC3.
// Refer to https://community.nxp.com/docs/DOC-94734

#ifndef RTC_H
#define RTC_H

#include "MKL25Z4.h"
#include "FreeRTOS.h"
#include "semphr.h"

typedef struct
{
    uint16_t  year;
    uint16_t  month;
    uint16_t  day;
    uint16_t  hour;
    uint16_t  minute;
    uint8_t   second;
    uint8_t   weekday;

}rtc_datetime_t;

extern SemaphoreHandle_t xRtcOneSecondSemaphore;
extern SemaphoreHandle_t xRtcAlarmSemaphore;

void rtc_init(void);
void rtc_get(rtc_datetime_t *datetime);
void rtc_set(rtc_datetime_t *datetime);

#endif
