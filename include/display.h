#pragma once

#include <Arduino.h>
#include <TimeLib.h>

// Can't simply include `time.h` because of conflict with Arduino's `time.h`.
// `esp_sntp.h` seems to include the right one.
#include <esp_sntp.h>

// Otherwise `Adafruit LEDBackpack` -> `Adafruit GFX` fails to build.
#include <Adafruit_I2CDevice.h>
#include <Adafruit_LEDBackpack.h>

#include "tz_info_local.h"

void displayBegin();
void display(time_t time, bool showColons, uint8_t brightness);
void displayUpdate(uint8_t currentBrightness, bool showColons);