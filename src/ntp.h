#pragma once

#include <Arduino.h>
#include <NTPClient.h>

#include "wifi.h"

#include "secrets.h"

time_t ntpGetEpochTime();