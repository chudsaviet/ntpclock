#pragma once

#include <time.h>
#include <Arduino.h>
#include <RTClib.h>

// Can't simply include `time.h` because of conflict with Arduino's `time.h`.
// `esp_sntp.h` seems to include the right one that have `timeval` defined.
#include <esp_sntp.h>

#include "tz_info_local.h"

bool beginRTC();

timeval getRTC();

void rtcSync();

void setRTC(timeval time);