#include "arduino_main.h"

#define TAG "arduino_main.cpp"

volatile uint8_t currentBrightness = DEFAULT_BRIGHTNESS;

uint32_t previousMillis = 0;
uint32_t lastRTCSync = 0;
volatile SyncSource lastSyncSource = SyncSource::none;

SemaphoreHandle_t i2cSemaphore = NULL;
TaskHandle_t xDisplayTaskHandle = NULL;

SyncSource getLastSyncSource()
{
  return lastSyncSource;
}

void adjustBrightness()
{
  float lux = alsGetLux(i2cSemaphore);
  ESP_LOGI(TAG, "ALS lux: %f", lux);

  float k = lux / MAX_LUX;
  currentBrightness = min(MAX_BRIGHTNESS,
                          MIN_BRIGNTNESS +
                              static_cast<uint8_t>(
                                  static_cast<float>(MAX_BRIGHTNESS - MIN_BRIGNTNESS) * k));
}

void arduinoSetup()
{
  i2cSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(i2cSemaphore);

  tzBegin();

  if (!beginRTC(i2cSemaphore))
  {
    ESP_LOGI(TAG, "Couldn't find RTC");
    abort();
  };

  // Do RTC sync before display and WiFi start to display time earlier.
  ESP_LOGI(TAG, "Performing early RTC sync.");
  rtcSync(i2cSemaphore);

  xTaskCreatePinnedToCore(
      vDisplayTask,
      "Display",
      configMINIMAL_STACK_SIZE * 2,
      &i2cSemaphore,
      tskIDLE_PRIORITY,
      &xDisplayTaskHandle,
      1);

  ntpBegin(i2cSemaphore);
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
      rtcSync(i2cSemaphore);
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