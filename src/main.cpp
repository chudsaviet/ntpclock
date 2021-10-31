#include <Arduino.h>

#include "wifi_local.h"
#include "ntp.h"
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

volatile uint8_t currentBrightness = DEFAULT_BRIGHTNESS;

uint32_t previousMillis = 0;
uint32_t lastBrightnessAdjustment = 0;
uint32_t lastDisplayUpdate = 0;

void adjustBrightness()
{
  float lux = alsGetLux();

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

  if (!beginRTC())
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
  };

  // Do RTC sync before display and WiFi start to display time earlier.
  Serial.println("Performing early RTC sync.");
  rtcSync();

  alsBegin();
  adjustBrightness();
  lastBrightnessAdjustment = millis();

  displayBegin();
  displayUpdate(currentBrightness);
  lastDisplayUpdate = millis();

  wifiBegin();

  ntpBegin();
}

void loop()
{
  uint32_t currentMillis = millis();
  if (currentMillis < previousMillis)
  {
    lastBrightnessAdjustment = 0;
    lastDisplayUpdate = 0;
  }

  if (currentMillis - lastDisplayUpdate > DISPLAY_UPDATE_INTERVAL_MS)
  {
    displayUpdate(currentBrightness);
    lastDisplayUpdate = currentMillis;
  }

  if (currentMillis - lastBrightnessAdjustment > BRIGHTNESS_ADJUST_INTERVAL_MS)
  {
    adjustBrightness();
    lastBrightnessAdjustment = currentMillis;
  }

  previousMillis = currentMillis;
  delay(HW_TICK_TIMER_INTERVAL_MS);
}