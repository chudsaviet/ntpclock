#include "arduino_main.h"

#define TAG "arduino_main.cpp"

volatile uint8_t currentBrightness = DEFAULT_BRIGHTNESS;

uint32_t previousMillis = 0;
uint32_t lastRTCSync = 0;
volatile SyncSource lastSyncSource = SyncSource::none;

SemaphoreHandle_t i2cSemaphoreArduinoMain = NULL;

SyncSource getLastSyncSource()
{
  return lastSyncSource;
}

void arduinoSetup(SemaphoreHandle_t i2cSemaphoreParameter)
{
  i2cSemaphoreArduinoMain = i2cSemaphoreParameter;

  tzBegin();

  if (!beginRTC(i2cSemaphoreArduinoMain))
  {
    ESP_LOGI(TAG, "Couldn't find RTC");
    abort();
  };

  // Do RTC sync before display and WiFi start to display time earlier.
  ESP_LOGI(TAG, "Performing early RTC sync.");
  rtcSync(i2cSemaphoreArduinoMain);

  ntpBegin(i2cSemaphoreArduinoMain);
}

void arduinoLoop()
{
  uint32_t currentMillis = millis();
  if (currentMillis < previousMillis)
  {
    lastRTCSync = 0;
    setLastNTPSyncMillis(0);
  }

  if (currentMillis - getLastNTPSyncMillis() > MAX_NTP_SYNC_INTERVAL_MS || !getNTPSyncHappened())
  {
    if (currentMillis - lastRTCSync > RTC_SYNC_INTERVAL_MS)
    {
      rtcSync(i2cSemaphoreArduinoMain);
      lastRTCSync = currentMillis;
      lastSyncSource = SyncSource::rtc;
    }
  }
  else
  {
    lastSyncSource = SyncSource::ntp;
  }

  previousMillis = currentMillis;
  vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_INTERVAL_MS));
}