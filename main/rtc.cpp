#include "rtc.h"

RTC_DS3231 rtc;

bool beginRTC(SemaphoreHandle_t i2cSemaphore)
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(BEGIN_RTC_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        bool result = rtc.begin();
        xSemaphoreGive(i2cSemaphore);

        return result;
    }
    else
    {
        Serial.println("beginRTC: couldn't take i2c semaphore.");
        return false;
    }
}

timeval getRTC(SemaphoreHandle_t i2cSemaphore)
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(ACCESS_RTC_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        DateTime rtcTime = rtc.now();
        xSemaphoreGive(i2cSemaphore);
        return {static_cast<time_t>(rtcTime.unixtime()), 0};
    }
    else
    {
        Serial.println("getRTC: couldn't take i2c semaphore.");
        timeval osTime;
        gettimeofday(&osTime, NULL);
        return osTime;
    }
}

void rtcSync(SemaphoreHandle_t i2cSemaphore)
{
    timeval rtcTime = getRTC(i2cSemaphore);
    settimeofday(&rtcTime, NULL);
}

void setRTC(timeval time, SemaphoreHandle_t i2cSemaphore)
{
    if (xSemaphoreTake(i2cSemaphore, pdMS_TO_TICKS(ACCESS_RTC_SEMAPHORE_TIMEOUT_MS)) == pdPASS)
    {
        Serial.print("Setting RTC to ");
        Serial.println(time.tv_sec);
        rtc.adjust(DateTime(time.tv_sec));
        xSemaphoreGive(i2cSemaphore);
    }
    else
    {
        Serial.println("setRTC: couldn't take i2c semaphore.");
    }
}