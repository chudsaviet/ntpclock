#include <Arduino.h>
#include <esp_sntp.h>

#include "wifi_local.h"
#include "ntp.h"
#include "display.h"
#include "rtc.h"
#include "als.h"

#define MAIN_LOOP_DELAY_INTERVAL_MS 128L
#define DISPLAY_UPDATE_INTERVAL_MS 32L
#define MAX_NTP_SYNC_INTERVAL_MS 1800000L // 30 min
#define RTC_SYNC_INTERVAL_MS 1800000L     // 30 min

#define BRIGHTNESS_ADJUST_INTERVAL_MS 1024L
#define DEFAULT_BRIGHTNESS 10L
#define MAX_BRIGHTNESS 16L
#define MIN_BRIGNTNESS 0L
#define MAX_LUX 500.0F

enum SyncSource
{
  none,
  rtc,
  ntp
};

volatile uint8_t currentBrightness = DEFAULT_BRIGHTNESS;

uint32_t previousMillis = 0;
uint32_t lastBrightnessAdjustment = 0;
uint32_t lastDisplayUpdate = 0;
uint32_t lastRTCSync = 0;
SyncSource lastSyncSource = SyncSource::none;

void adjustBrightness()
{
  float lux = alsGetLux();
  Serial.print("ALS lux: ");
  Serial.println(lux);

  float k = lux / MAX_LUX;
  currentBrightness = min(MAX_BRIGHTNESS,
                          MIN_BRIGNTNESS +
                              static_cast<uint8_t>(
                                  static_cast<float>(MAX_BRIGHTNESS - MIN_BRIGNTNESS) * k));
}

void setup()
{
  Serial.begin(9600);
  delay(4000);

  tzBegin();

  if (!beginRTC())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  };

  // Do RTC sync before display and WiFi start to display time earlier.
  Serial.println("Performing early RTC sync.");
  rtcSync();

  displayBegin();
  displayUpdate(currentBrightness, false);
  lastDisplayUpdate = millis();

  wifiBegin();

  ntpBegin();

  alsBegin();
  adjustBrightness();
  lastBrightnessAdjustment = millis();
}

void loop()
{
  uint32_t currentMillis = millis();
  if (currentMillis < previousMillis)
  {
    lastBrightnessAdjustment = 0;
    lastDisplayUpdate = 0;
    lastRTCSync = 0;
    setLastNTPSyncMillis(0);
  }

  if (currentMillis - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL_MS)
  {
    displayUpdate(currentBrightness, lastSyncSource == SyncSource::ntp);
    lastDisplayUpdate = currentMillis;
  }

  if (currentMillis - lastBrightnessAdjustment > BRIGHTNESS_ADJUST_INTERVAL_MS)
  {
    adjustBrightness();
    lastBrightnessAdjustment = currentMillis;
  }

  if (currentMillis - getLastNTPSyncMillis() > MAX_NTP_SYNC_INTERVAL_MS)
  {
    if (currentMillis - lastRTCSync > RTC_SYNC_INTERVAL_MS)
    {
      rtcSync();
      lastRTCSync = currentMillis;
      lastSyncSource = SyncSource::rtc;
    }
  }
  else
  {
    lastSyncSource = SyncSource::ntp;
  }

  previousMillis = currentMillis;
  delay(MAIN_LOOP_DELAY_INTERVAL_MS);
}