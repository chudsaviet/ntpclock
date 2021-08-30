#include <time.h>
#include <Arduino.h>
#include <TimeLib.h>
#include <TimeZoneInfo.h>
#include <SAMDTimerInterrupt.h>

#include "wifi.h"
#include "ntp.h"
#include "timezone.h"
#include "display.h"
#include "rtc.h"

#define HW_TICK_TIMER_INTERVAL_MS 100L
#define NTP_SYNC_INTERVAL_MS 3600000L
#define RTC_SYNC_INTERVAL_MS 10000L

volatile timespec currentTime = {0, 0};

SAMDTimer TickTimer(TIMER_TCC);

TimeZoneInfo tzInfo;

uint32_t lastNTPSync = 0;
uint32_t lastRTCSync = 0;

bool currentTimeInc(unsigned long inc_ms)
{
  currentTime.tv_nsec += HW_TICK_TIMER_INTERVAL_MS * 1000000L;
  if (currentTime.tv_nsec > 1000000000L)
  {
    currentTime.tv_sec += 1LL;
    currentTime.tv_nsec -= 1000000000L;
    return true;
  }
  else
  {
    return false;
  }
}

void tick()
{
  if (currentTimeInc(HW_TICK_TIMER_INTERVAL_MS))
  {
    display(currentTime.tv_sec);
  }
}

void rtcSync()
{
  timespec rtcTime = getRTC();
  rtcTime.tv_sec = tzInfo.utc2local(rtcTime.tv_sec);

  currentTime.tv_sec = rtcTime.tv_sec;
  currentTime.tv_nsec = rtcTime.tv_nsec;
}

void ntpSync()
{
  timespec ntpTime = ntpGetTime();

  currentTime.tv_nsec = ntpTime.tv_nsec;
  currentTime.tv_sec = tzInfo.utc2local(ntpTime.tv_sec);

  setRTC(ntpTime);
}

void setup()
{
  Serial.begin(9600);

  tzInfo.setLocation_P(tzData);

  if (!beginRTC())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  }

  rtcSync();
  lastRTCSync = millis();

  displayBegin();

  TickTimer.attachInterruptInterval(HW_TICK_TIMER_INTERVAL_MS * 1000, tick);

  wifiBegin();
  wifiLowPower();

  ntpSync();
  lastNTPSync = millis();
}

void loop()
{
  if (millis() - lastNTPSync > NTP_SYNC_INTERVAL_MS)
  {
    ntpSync();
    lastNTPSync = millis();
  }
  //if (millis() - lastRTCSync > RTC_SYNC_INTERVAL_MS)
  //{
  //  rtcSync();
  //  lastRTCSync = millis();
  //}
  delay(4000);
}