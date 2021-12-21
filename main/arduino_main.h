#pragma once

#include <Arduino.h>
#include <esp_log.h>

#include "rtc.h"

#define ARDUINO_LOOP_DELAY_INTERVAL_MS 1000L
#define DISPLAY_UPDATE_INTERVAL_MS 32L
#define MAX_NTP_SYNC_INTERVAL_MS 7200000L // 2 hours
#define RTC_SYNC_INTERVAL_MS 1800000L     // 30 min

#define BRIGHTNESS_ADJUST_INTERVAL_MS 1024L
#define DEFAULT_BRIGHTNESS 10L

void arduinoSetup(SemaphoreHandle_t i2cSemaphore);
void arduinoLoop();
