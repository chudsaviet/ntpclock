#pragma once

#include <time.h>
#include <Arduino.h>
#include <esp_sntp.h>

#include "wifi_local.h"
#include "rtc.h"

#include "secrets.h"

void ntpBegin();

uint32_t getLastNTPSyncMillis();
void setLastNTPSyncMillis(uint32_t m);
