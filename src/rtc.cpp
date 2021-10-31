#include "rtc.h"

RTC_DS3231 rtc;

bool beginRTC()
{
    return rtc.begin();
}

timeval getRTC()
{
    DateTime rtcTime = rtc.now();
    return {static_cast<time_t>(rtcTime.unixtime()), 0};
}

void rtcSync()
{
    timeval rtcTime = getRTC();
    rtcTime.tv_sec = utc2local(rtcTime.tv_sec);

    settimeofday(&rtcTime, {0});
}

void setRTC(timeval time)
{
    Serial.print("Setting RTC to ");
    Serial.println(time.tv_sec);
    rtc.adjust(DateTime(time.tv_sec));
}