#pragma once

#include <time.h>
#include <Arduino.h>
#include <RTClib.h>

// Can't simply include `time.h` because of conflict with Arduino's `time.h`.
// `esp_sntp.h` seems to include the right one that have `timeval` defined.
#include <esp_sntp.h>

#include "tz_info_local.h"

#define BEGIN_RTC_SEMAPHORE_TIMEOUT_MS 2048L

#define ACCESS_RTC_SEMAPHORE_TIMEOUT_MS 128L

bool beginRTC(SemaphoreHandle_t i2cSemaphore);

timeval getRTC(SemaphoreHandle_t i2cSemaphore);

void rtcSync(SemaphoreHandle_t i2cSemaphore);

void setRTC(timeval time, SemaphoreHandle_t i2cSemaphore);