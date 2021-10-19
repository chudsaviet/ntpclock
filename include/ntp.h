#pragma once

#include <time.h>
#include <Arduino.h>
#include <NTPClient.h>

#include "wifi_local.h"

#include "secrets.h"

#define NTP_ERA 0

timespec ntpGetTime();