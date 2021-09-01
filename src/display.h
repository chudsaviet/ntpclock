#pragma once

#include <Arduino.h>
#include <TimeLib.h>

// Otherwise `Adafruit LEDBackpack` -> `Adafruit GFX` fails to build.
#include <Adafruit_I2CDevice.h>

#include <Adafruit_LEDBackpack.h>

void displayBegin();
void display(time_t time, bool showColons);