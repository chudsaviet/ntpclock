#include "arduino_main.h"

#define TAG "arduino_main.cpp"

volatile uint8_t currentBrightness = DEFAULT_BRIGHTNESS;

uint32_t previousMillis = 0;
uint32_t lastRTCSync = 0;

SemaphoreHandle_t i2cSemaphoreArduinoMain = NULL;

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
}

void arduinoLoop()
{
  vTaskDelay(pdMS_TO_TICKS(ARDUINO_LOOP_DELAY_INTERVAL_MS));
}