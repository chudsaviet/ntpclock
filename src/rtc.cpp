#include "rtc.h"

RTC_DS3231 rtc;

bool beginRTC()
{
    return rtc.begin();
}

timespec getRTC()
{
    // TODO: implemnt sub-second precision in RTC.
    DateTime rtcTime = rtc.now();
    return {static_cast<time_t>(rtcTime.unixtime()), 0};
}

void setRTC(timespec time)
{
    Serial.print("Setting RTC to ");
    Serial.println(time.tv_sec);
    rtc.adjust(DateTime(time.tv_sec));
}