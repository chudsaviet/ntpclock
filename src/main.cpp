#include <time.h>
#include <Arduino.h>
#include <TimeLib.h>
#include <TimeZoneInfo.h>
#include <SAMDTimerInterrupt.h>

#include "wifi.h"
#include "ntp.h"
#include "timezone.h"
#include "display.h"

#define HW_TICK_TIMER_INTERVAL_MS 100L

volatile timespec currentTime = {0, 0};

SAMDTimer TickTimer(TIMER_TCC);

TimeZoneInfo tzInfo;

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
    Serial.println(currentTime.tv_sec);
  }
}

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  displayBegin();

  TickTimer.attachInterruptInterval(HW_TICK_TIMER_INTERVAL_MS * 1000, tick);

  wifiBegin();
  wifiLowPower();

  tzInfo.setLocation_P(tzData);
}

void loop()
{
  timespec ntpTime = ntpGetTime();
  ntpTime.tv_sec = tzInfo.utc2local(ntpTime.tv_sec);

  currentTime.tv_sec = ntpTime.tv_sec;
  currentTime.tv_nsec = ntpTime.tv_nsec;
  delay(300000);
}