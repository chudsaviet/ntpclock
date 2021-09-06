#pragma once

#include <time.h>
#include <Arduino.h>
#include <RTClib.h>

bool beginRTC();

timespec getRTC();

void setRTC(timespec time);