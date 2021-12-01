#pragma once

#include <time.h>
#include <Arduino.h>
#include <esp_sntp.h>
#include <esp_log.h>

#include "rtc.h"

#include "secrets.h"

void ntpBegin(SemaphoreHandle_t i2cSemaphoreExternal);

bool getNTPSyncHappened();
uint32_t getLastNTPSyncMillis();
void setLastNTPSyncMillis(uint32_t m);
