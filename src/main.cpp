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
#include "als.h"

#define HW_TICK_TIMER_INTERVAL_MS 10L
#define DISPLAY_UPDATE_INTERVAL_MS 32L
#define CLOCK_SYNC_INTERVAL_MS 1048576L
#define BRIGHTNESS_ADJUST_INTERVAL_MS 1024L
#define DEFAULT_BRIGHTNESS 8L
#define MAX_BRIGHTNESS 16L
#define MIN_BRIGNTNESS 0L
#define MAX_LUX 500.0F

enum SyncSource
{
  none,
  rtc,
  ntp
};

// RTC and NTP carry UTC time, currentTime carries local time.
volatile timespec currentTime = {0, 0};
volatile uint8_t currentBrightness = DEFAULT_BRIGHTNESS;

volatile SyncSource lastSyncSource = SyncSource::none;

SAMDTimer TickTimer(TIMER_TCC);

TimeZoneInfo tzInfo;

uint32_t previousMillis = 0;
uint32_t lastSync = 0;
uint32_t lastBrightnessAdjustment = 0;
uint32_t lastDisplayUpdate = 0;

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
  currentTimeInc(HW_TICK_TIMER_INTERVAL_MS);
}

void displayUpdate()
{
  display(currentTime.tv_sec, lastSyncSource == SyncSource::ntp, currentBrightness);
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

void adjustBrightness()
{
  float lux = alsGetLux();
  Serial.print("Got ");
  Serial.print(lux);
  Serial.println(" lux.");

  float k = lux / MAX_LUX;
  currentBrightness = min(MAX_BRIGHTNESS,
                          MIN_BRIGNTNESS +
                              static_cast<uint8_t>(
                                  static_cast<float>(MAX_BRIGHTNESS - MIN_BRIGNTNESS) * k));
  Serial.print("Setting brightness to ");
  Serial.println(currentBrightness);
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

  alsBegin();
  adjustBrightness();
  lastBrightnessAdjustment = millis();

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
  uint32_t currentMillis = millis();
  if (currentMillis < previousMillis)
  {
    lastSync = 0;
    lastBrightnessAdjustment = 0;
    lastDisplayUpdate = 0;
  }

  if (currentMillis - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL_MS)
  {
    displayUpdate();
    lastDisplayUpdate = currentMillis;
  }

  if (currentMillis - lastBrightnessAdjustment > BRIGHTNESS_ADJUST_INTERVAL_MS)
  {
    adjustBrightness();
    lastBrightnessAdjustment = currentMillis;
  }

  if (currentMillis - lastSync > CLOCK_SYNC_INTERVAL_MS)
  {
    sync();
    lastSync = currentMillis;
  }

  previousMillis = currentMillis;
  delay(8);
}