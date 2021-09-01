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
#define CLOCK_SYNC_INTERVAL_MS 1048576L

enum SyncSource
{
  none,
  rtc,
  ntp
};

// RTC and NTP carry UTC time, currentTime carries local time.
volatile timespec currentTime = {0, 0};

volatile SyncSource lastSyncSource = SyncSource::none;

SAMDTimer TickTimer(TIMER_TCC);

TimeZoneInfo tzInfo;

// We don't care about millis() overflow,
// the worst this that will happen is just one additional sync.
uint32_t lastSync = 0;

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
    display(currentTime.tv_sec, lastSyncSource == SyncSource::ntp);
  }
}

void rtcSync()
{
  timespec rtcTime = getRTC();
  rtcTime.tv_sec = tzInfo.utc2local(rtcTime.tv_sec);

  currentTime.tv_sec = rtcTime.tv_sec;
  // Not touching currentTime.tv_nsec because RTC does not offer subsecond precision.

  lastSyncSource = SyncSource::rtc;
}

bool ntpSync()
{
  timespec ntpTime = ntpGetTime();

  if (ntpTime.tv_sec != 0 || ntpTime.tv_nsec != 0)
  {
    currentTime.tv_nsec = ntpTime.tv_nsec;
    currentTime.tv_sec = tzInfo.utc2local(ntpTime.tv_sec);
    lastSyncSource = SyncSource::ntp;
    setRTC(ntpTime);
    return true;
  }
  else
  {
    return false;
  }
}

void sync()
{
  Serial.println("Attempting NTP sync...");
  if (ntpSync())
  {
    Serial.println("NTP sync successful.");
  }
  else
  {
    Serial.println("NTP sync failed.");
    Serial.println("Performing RTC sync.");
    rtcSync();
  }
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

  // Do RTC sync before display and WiFi start to display time earlier.
  Serial.println("Performing early RTC sync.");
  rtcSync();
  lastSync = millis();

  displayBegin();

  TickTimer.attachInterruptInterval(HW_TICK_TIMER_INTERVAL_MS * 1000, tick);

  wifiBegin();
  wifiLowPower();

  // Performing first full sync.
  sync();
  lastSync = millis();
}

void loop()
{
  if (millis() - lastSync > CLOCK_SYNC_INTERVAL_MS)
  {
    sync();
    lastSync = millis();
  }
  delay(4000);
}